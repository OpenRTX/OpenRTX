/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "SDL2/SDL.h"

#include "readline/readline.h"
#include "readline/history.h"

#include "emulator.h"
#include "sdl_engine.h"

/* Custom SDL Event to request a screenshot */
extern Uint32 SDL_Screenshot_Event;

emulator_state_t emulator_state =
{
    -100.0f,  // RSSI
    8.2f,     // Vbat
    3,        // mic level
    4,        // volume level
    1,        // chSelector
    false,    // PTT status
    false     // power off
};

typedef int (*_climenu_fn)(void *self, int argc, char **argv);

typedef struct
{
    char *name;
    char *description;
    void *var;
    _climenu_fn fn;
}
_climenu_option;

enum shell_retvals
{
    SH_ERR = -1,
    SH_CONTINUE = 0,
    SH_WHAT = 1,
    SH_EXIT_OK = 2,
};

static keyboard_t _shellkeyq[25] = {0};
static int _skq_cap = 25;
static int _skq_head;
static int _skq_tail;
static int _skq_in;
static int _skq_out;

// NOTE: unused function
// static void _dump_skq()
// {
//     for(int i = 0; i < _skq_cap; i++)
//     {
//         printf("skq[%d] == %d\n", i, _shellkeyq[i]);
//     }
// }

// NOTE: unused function
// static void _test_skq()
// {
//     for(int i = 0; i < 257; i++)
//     {
//         shellkeyq_put(i + 1);
//     }
//
//     //clear it out now
//     while(emulator_getKeys());
// }

static void shellkeyq_put(keyboard_t keys)
{
    // note - we must allow keys == 0 to be inserted because otherwise a queue
    // full of [1,1,1,1,1] is simulating HOLDING 1, and we sometimes
    // (well, often) want [1,0,1,0,1,0] to simulate separate keypresses
    // this, of course, relies on the kbd_thread getting just one element off
    // the queue for every kbd_getKeys().
    if(_skq_in > _skq_out + _skq_cap)
    {
        printf("too many keys!\n");
        return;
    }

    _shellkeyq[ _skq_tail ] = keys;
    _skq_in++;
    _skq_tail = (_skq_tail + 1) % _skq_cap;
}

static int shell_ready(void *_self, int _argc, char **_argv)
{
    (void) _self;
    (void) _argc;
    (void) _argv;

    while(_skq_in > _skq_out)
    {
        usleep(10 * 1000); //sleep until keyboard is caught up
    }
    return SH_CONTINUE;
}

static keyboard_t keyname2keyboard(char *name)
{
    /*  The line noise at the end of this comment is a vim macro for taking the
     *  keyboard.h interface and putting it into the format further below.
     *  You can load it into vim register k with "kyy and run the macro with @k
     *  (and then you can repeat a macro register application with @@ )
     *  (substitute k with any register you like)
     *  Once you've got all the names quoted, you can J them all together into
     *  a nice block.
     *
     *   _i"ElC",
     *
     */
    char *names[] =
    {
        "KEY_0", "KEY_1", "KEY_2", "KEY_3", "KEY_4", "KEY_5", "KEY_6", "KEY_7",
        "KEY_8", "KEY_9", "KEY_STAR", "KEY_HASH", "KEY_ENTER", "KEY_ESC",
        "KEY_UP","KEY_DOWN", "KEY_LEFT", "KEY_RIGHT", "KEY_MONI", "KEY_F1",
        "KEY_F2", "KEY_F3", "KEY_F4", "KEY_F5", "KEY_F6", "KEY_VOLUP", "KEY_VOLDOWN",
        "KNOB_LEFT", "KNOB_RIGHT",
    };

    int numnames = sizeof(names) / sizeof(char *);

    for(int i = 0; i < numnames; i++)
    {
       /*
        * +4 to skip the KEY_ on all the names, non +4 to allow for KNOB_LEFT.
        * This also means you can write KEY_LEFT as "KEY_LEFT", or "LEFT" and
        * KNOB_LEFT as "KNOB_LEFT" or "_LEFT"
        *
        * so if name == "2", this whole function will return equivalent to KEY_2 cpp define
        * and if name=="LEFT", then you get equivalent to KEY_LEFT cpp define
        * and if name=="_LEFT", then you get equivalent to KNOB_LEFT cpp define
        * and if name=="KNOB_LEFT", then you get equivalent to KNOB_LEFT cpp define
        * and if name=="KEY_2", then you get equivalent to KEY_2 cpp define.
        *
        * Of course order matters a great deal in names array, has to match the
        * bit field generated in interface/keyboard.h so double check that with
        * every update
        */

        if((strcasecmp(name, names[i] + 4) == 0) ||
           (strcasecmp(name, names[i]) == 0))       //notice case insensitive
        {
            return (1 << i);
        }
    }
    return 0;
}

static int pressKey(void *_self, int _argc, char **_argv)
{
    (void) _self;

    printf("Press Keys: [\n");
    keyboard_t last = 0;

    for(int i = 0; i < _argc; i++)
    {
        if(_argv[i] != NULL)
        {
            printf("\t%s, \n", _argv[i]);
            keyboard_t press = keyname2keyboard(_argv[i]);
            if(press == last)
            {
                /* otherwise if you send key ENTER DOWN DOWN DOWN DOWN DOWN
                 * it will just hold DOWN for (5/(kbd_task_hz)) seconds
                 * so we need to give it a 0 value to get a 'release'
                 * so the next input is recognized as separate
                 * we only need to do this if we have two identical keys back
                 * to back, because keyboard_t will have a zero for this key's
                 * flag on other keys, which gives us the release we need
                 */
                shellkeyq_put(0);
            }
            shellkeyq_put(press);
            last = press;
        }
    }
    printf("\t]\n");
    shell_ready(NULL, 0, NULL);
    return SH_CONTINUE; // continue
}

// pressMultiKeys allows for key combos by sending all the keys specified in
// one keyboard_t
static int pressMultiKeys(void *_self, int _argc, char **_argv)
{
    (void) _self;
    printf("Press Keys: [\n");
    keyboard_t combo = 0;

    for(int i = 0; i < _argc; i++)
    {
        if(_argv[i] != NULL)
        {
            printf("\t%s, \n", _argv[i]);
            combo |= keyname2keyboard(_argv[i]);
        }
    }

    shellkeyq_put(combo);
    printf("\t]\n");
    shell_ready(NULL, 0, NULL);
    return SH_CONTINUE; // continue
}

// NOTE: unused function
// static int template(void *_self, int _argc, char **_argv)
// {
//     _climenu_option *self = (_climenu_option *) _self;
//     printf("%s\n\t%s\n", self->name, self->description);
//
//     for(int i = 0; i < _argc; i++)
//     {
//         if(_argv[i] != NULL)
//         {
//             printf("\tArgs:\t%s\n", _argv[i]);
//         }
//     }
//
//     return SH_CONTINUE; // continue
// }

static int screenshot(void *_self, int _argc, char **_argv)
{
    (void) _self;
    char *filename = "screenshot.bmp";

    if(_argc && _argv[0] != NULL)
    {
        filename = _argv[0];
    }

    int len = strlen(filename);

    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_Screenshot_Event;
    e.user.code = 0;        /* async: do not post the handshake semaphore */
    e.user.data1 = malloc(len+1);
    memset(e.user.data1, 0, len+1);
    strcpy(e.user.data1, filename);

    return SDL_PushEvent(&e) == 1 ? SH_CONTINUE : SH_ERR;
}

/* Default timeout for screenshot_sync, in milliseconds.  Generous
 * because the SDL thread may be busy servicing input/render events when
 * the request is enqueued. */
#define SCREENSHOT_SYNC_TIMEOUT_MS 5000

static int screenshot_sync(void *_self, int _argc, char **_argv)
{
    (void) _self;
    char *filename = "screenshot.bmp";

    if(_argc && _argv[0] != NULL)
    {
        filename = _argv[0];
    }

    int len = strlen(filename);

    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_Screenshot_Event;
    e.user.code = 1;        /* sync: SDL thread will post the handshake */
    e.user.data1 = malloc(len+1);
    memset(e.user.data1, 0, len+1);
    strcpy(e.user.data1, filename);

    if(SDL_PushEvent(&e) != 1)
    {
        free(e.user.data1);
        return SH_ERR;
    }

    if(!sdlEngine_waitScreenshot(SCREENSHOT_SYNC_TIMEOUT_MS))
    {
        printf("screenshot_sync: timed out after %d ms waiting for %s\n",
               SCREENSHOT_SYNC_TIMEOUT_MS, filename);
        return SH_ERR;
    }

    return SH_CONTINUE;
}

static int setFloat(void *_self, int _argc, char **_argv)
{
    _climenu_option *self = (_climenu_option *) _self;

    if(_argc <= 0 || _argv[0] == NULL)
    {
        printf("%s is %f\n", self->name,  *(float *)(self->var));
    }
    else
    {
        sscanf(_argv[0], "%f", (float *)self->var);
        printf("%s is %f\n", self->name,  *(float *)(self->var));
    }

    return SH_CONTINUE; // continue

}

static int toggleVariable(void *_self, int _argc, char **_argv)
{
    (void) _argc;
    (void) _argv;
    _climenu_option *self = (_climenu_option *) _self;
    *(int *)self->var = ! *(int *)self->var; //yeah, maybe this got a little out of hand

    return SH_CONTINUE; // continue
}

static int shell_sleep(void *_self, int _argc, char **_argv)
{
    (void) _self;

    if(! _argc || _argv[0] == NULL)
    {
        printf("Provide a number in milliseconds to sleep as an argument\n");
        return SH_ERR;
    }

    useconds_t sleepus = atoi(_argv[0]) * 1000;
    usleep(sleepus);
    return SH_CONTINUE;
}

/* wait_stable defaults: poll the framebuffer hash every 50ms and
 * return as soon as N consecutive polls produce the same hash, or the
 * overall timeout elapses.  Callers can override both bounds. */
#define WAIT_STABLE_DEFAULT_MAX_MS  2000
#define WAIT_STABLE_DEFAULT_POLLS   3
#define WAIT_STABLE_INTERVAL_MS     50

static int shell_wait_stable(void *_self, int _argc, char **_argv)
{
    (void) _self;

    int max_ms       = WAIT_STABLE_DEFAULT_MAX_MS;
    int stable_polls = WAIT_STABLE_DEFAULT_POLLS;

    if(_argc >= 1 && _argv[0] != NULL)
    {
        max_ms = atoi(_argv[0]);
        if (max_ms <= 0) max_ms = WAIT_STABLE_DEFAULT_MAX_MS;
    }
    if(_argc >= 2 && _argv[1] != NULL)
    {
        stable_polls = atoi(_argv[1]);
        if (stable_polls < 2) stable_polls = 2;
    }

    uint64_t prev = 0;
    bool     have_prev = false;
    int      consecutive = 0;
    int      elapsed = 0;

    while (elapsed < max_ms)
    {
        uint64_t h = 0;
        if (!sdlEngine_getFrameHash(&h, 0))
        {
            printf("wait_stable: framehash failed; aborting\n");
            return SH_ERR;
        }

        if (have_prev && h == prev)
        {
            consecutive++;
            if (consecutive >= stable_polls)
            {
                return SH_CONTINUE;
            }
        }
        else
        {
            consecutive = 1;
        }
        prev = h;
        have_prev = true;

        usleep(WAIT_STABLE_INTERVAL_MS * 1000);
        elapsed += WAIT_STABLE_INTERVAL_MS;
    }

    printf("wait_stable: timed out after %d ms (frame still changing)\n",
           max_ms);
    /* Non-fatal: return CONTINUE so the test proceeds and the
     * subsequent screenshot diff will surface the real problem. */
    return SH_CONTINUE;
}

static int shell_quit( void *_self, int _argc, char **_argv)
{
    (void) _self;
    (void) _argc;
    (void) _argv;
    printf("QUIT: 73!\n");

    //could remove history entries here, if we wanted
    return SH_EXIT_OK; //normal quit
}

static int printState( void *_self, int _argc, char **_argv)
{
    (void) _self;
    (void) _argc;
    (void) _argv;
    printf("\nCurrent state\n");
    printf("RSSI   : %f\n",   emulator_state.RSSI);
    printf("Battery: %f\n",   emulator_state.vbat);
    printf("Mic    : %f\n",   emulator_state.micLevel);
    printf("Volume : %f\n",   emulator_state.volumeLevel);
    printf("Channel: %f\n",   emulator_state.chSelector);
    printf("PTT    : %s\n\n", emulator_state.PTTstatus ? "true" : "false");
    return SH_CONTINUE;
}

static int shell_nop( void *_self, int _argc, char **_argv)
{
    (void) _self;
    (void) _argc;
    (void) _argv;
    //do nothing! what it says on the tin
    return SH_CONTINUE;
}

// Forward declaration needed to include function pointer in the table below
static int shell_help( void *_self, int _argc, char **_argv);

static _climenu_option _options[] =
{
    /* name/shortcut   description            var reference, if available    method to call */
    {"rssi",    "Set rssi",     (void *) &emulator_state.RSSI,        setFloat },
    {"vbat",    "Set vbat",     (void *) &emulator_state.vbat,        setFloat },
    {"mic",     "Set miclevel", (void *) &emulator_state.micLevel,    setFloat },
    {"volume",  "Set volume",   (void *) &emulator_state.volumeLevel, setFloat },
    {"channel", "Set channel",  (void *) &emulator_state.chSelector,  setFloat },
    {"ptt",     "Toggle PTT",   (void *) &emulator_state.PTTstatus,   toggleVariable },
    {"key",     "Press keys in sequence (e.g. 'key ENTER DOWN ENTER' will descend through two menus)",
                                NULL,   pressKey
    },
    {"keycombo", "Press a bunch of keys simultaneously", NULL, pressMultiKeys },
    {"show",     "Show current radio state (ptt, rssi, etc)", NULL, printState},
    {"screenshot", "[screenshot.bmp] Save screenshot to first arg or screenshot.bmp if none given",
                                NULL,   screenshot
    },
    {"screenshot_sync", "[screenshot.bmp] Like screenshot, but block until the BMP is written",
                                NULL,   screenshot_sync
    },
    {"sleep",   "Wait some number of ms",           NULL,   shell_sleep },
    {"wait_stable", "[max_ms=2000] [stable_polls=3] Wait until the framebuffer is stable",
                                NULL,   shell_wait_stable
    },
    {"help",    "Print this help",                  NULL,   shell_help },
    {"nop",     "Do nothing (useful for comments)", NULL,   shell_nop},
    {"quit",    "Quit, close the emulator",         NULL,   shell_quit },
    /*{"ready",     */
    /*"Wait until ready. Currently supports keyboard, so will wait until all keyboard events are processed,"*/
    /*"but is already implied by key and keycombo so there's not much direct use for it right now",*/
    /*NULL,   shell_ready },*/
};

static const int num_options = (sizeof(_options) / sizeof(_climenu_option));

static int shell_help( void *_self, int _argc, char **_argv)
{
    (void) _self;
    (void) _argc;
    (void) _argv;
    printf("OpenRTX emulator shell\n\n");

    for(int i = 0; i < num_options; i++)
    {
        _climenu_option *o = &_options[i];
        printf("%10s -> %s\n", o->name, o->description);
    }

    return SH_CONTINUE;
}

static _climenu_option *findMenuOption(char *tok)
{
    for(int i = 0; i < num_options; i++)
    {
        _climenu_option *o = &_options[i];

        /*
         * strncmp like this allows for typing shortcuts like just "r" instead
         * of the full "rssi". Priority for conflicts (like if there's "s"
         * which could mean either "show" or "screenshot" ) is set by ordering
         * in the _options array
         */
        if(strncmp(tok, o->name, strlen(tok)) == 0)
        {
            return o;
        }
    }

    return NULL;
}

static void striptoken(char *token)
{
    for(size_t i = 0; i < strlen(token); i++)
    {
        if(token[i] == '\n')
        {
            token[i] = 0;
        }
    }
}

static int process_line(char *line)
{
    char *token = strtok(line, " ");

    if(token == NULL)
    {
        return SH_ERR;
    }

    striptoken(token);
    _climenu_option *o = findMenuOption(token);
    char *args[12] = {NULL};
    int i = 0;

    for(i = 0; i < 12; i++)
    {
        //immediately strtok again since first is a command rest are args
        token = strtok(NULL, " ");
        if(token == NULL)
        {
            break;
        }
        striptoken(token);
        args[i] = token;
    }

    if(token != NULL)
    {
        printf("\nGot too many arguments, args truncated \n");
    }

    if(o != NULL)
    {
        if(o->fn != NULL)
        {
            return o->fn(o, i, args);
        }
        else
        {
            printf("Bad fn for o, check option array for bad data\n");
            return SH_ERR;
        }
    }
    else
    {
        return SH_WHAT; //not understood
    }
}

void *startCLIMenu(void *arg)
{
    (void) arg;

    printf("\n\n");
    char *histfile = ".emulatorsh_history";
    shell_help(NULL, 0, NULL);
    int ret = SH_CONTINUE;
    using_history();
    read_history(histfile);

    do
    {
        char *r = readline(">");

        if(r == NULL)
        {
            ret = SH_EXIT_OK;
        }
        else if(strlen(r) > 0)
        {
            add_history(r);
            ret = process_line(r);
        }
        else
        {
            ret = SH_CONTINUE;
        }

        switch(ret)
        {
            default:
                fflush(stdout);
                break;

            case SH_WHAT:
                printf("?\n(type h or help for help)\n");
                ret = SH_CONTINUE;
                /*printf("\n>");*/
                break;

            case SH_CONTINUE:
                /*printf("\n>");*/
                break;

            case SH_EXIT_OK:
                //normal quit
                emulator_state.powerOff = true;
                break;

            case SH_ERR:
                //error
                printf("Error running that command\n");
                ret = SH_CONTINUE;
                break;
        }

        free(r); //free the string allocated by readline
    }
    while((ret == SH_CONTINUE) && (emulator_state.powerOff == false));

    fflush(stdout);
    write_history(histfile);

    return NULL;
}



void emulator_start()
{
    sdlEngine_init();

    pthread_t cli_thread;
    int err = pthread_create(&cli_thread, NULL, startCLIMenu, NULL);

    if(err)
    {
        printf("An error occurred starting the emulator CLI thread: %d\n", err);
    }
}

keyboard_t emulator_getKeys()
{
    if(_skq_in > _skq_out)
    {
        //only if we've fallen behind and there's data in there:
        keyboard_t out = _shellkeyq[ _skq_head ];
        _shellkeyq[ _skq_head ] = 0;
        _skq_out++;
        _skq_head = (_skq_head + 1) % _skq_cap;
        return out;
    }
    else
    {
        return 0; //no keys
    }
}
