
#include  <app_cfg.h>
#include  <os.h>
#include  <stdio.h>
#include  <lib_mem.h>
#include  <lib_math.h>
#include <SDL2/SDL.h>
#undef main

#include <interfaces/graphics.h>

static OS_TCB        App_TaskStartTCB;
static CPU_STK_SIZE  App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static void App_TaskStart(void *p_arg);

static OS_TCB        gfxTCB;
static CPU_STK_SIZE  gfxStk[APP_CFG_TASK_START_STK_SIZE];
static void gfxThread(void *arg);

static int running = 0;

int  main (void)
{
    OS_ERR  err;
    running = 1;

    OSInit(&err);
    OS_CPU_SysTickInit();

    OSTaskCreate((OS_TCB     *)&App_TaskStartTCB,
                 (CPU_CHAR   *)"App Task Start",
                 (OS_TASK_PTR ) App_TaskStart,
                 (void       *) 0,
                 (OS_PRIO     ) APP_CFG_TASK_START_PRIO,
                 (CPU_STK    *)&App_TaskStartStk[0],
                 (CPU_STK     )(APP_CFG_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

     OSTaskCreate((OS_TCB     *)&gfxTCB,
                 (CPU_CHAR   *)"gfx",
                 (OS_TASK_PTR ) gfxThread,
                 (void       *) 0,
                 (OS_PRIO     ) APP_CFG_TASK_START_PRIO,
                 (CPU_STK    *)&gfxStk[0],
                 (CPU_STK     )(APP_CFG_TASK_START_STK_SIZE / 10u),
                 (CPU_STK_SIZE) APP_CFG_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  ) 0,
                 (OS_TICK     ) 0,
                 (void       *) 0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    OSStart(&err);

    printf("OSStart returned, quitting\n");
    return 0;
}

static void App_TaskStart(void *p_arg)
{

    (void) p_arg;
    OS_ERR os_err;

//     OS_CPU_SysTickInit();

    while (running)
    {
        printf("uCOS-III is running.\n");
        OSTimeDlyHMSM(0u, 0u, 1u, 0u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }

    exit(1);
}

static void gfxThread(void *arg)
{
    (void) arg;
    OS_ERR os_err;

    int pos = 0;
    SDL_Event eventListener;

    gfx_init();

    while(1)
    {
        SDL_PollEvent(&eventListener);
        if(eventListener.type == SDL_QUIT) break;

        gfx_clearScreen();
        point_t origin = {0, pos};
        color_t color_red = {255, 0, 0};
        color_t color_white = {255, 255, 255};
        gfx_drawRect(origin, SCREEN_WIDTH, 20, color_red, 1);
        gfx_print(origin, FONT_SIZE_4, TEXT_ALIGN_LEFT, 
                  color_white, "KEK");
        gfx_render();
        while(gfx_renderingInProgress()) ;
        pos += 20;
        if(pos > SCREEN_HEIGHT - 20) pos = 0;

        OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    }

    running = 0;
    OSTimeDlyHMSM(0u, 0u, 0u, 100u, OS_OPT_TIME_HMSM_STRICT, &os_err);
    gfx_terminate();
}
