#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdio.h>

#include "csk_os_memory.h"
#include "csk_os_timer.h"

static struct k_work_q xr_ostimer_workq;
K_THREAD_STACK_DEFINE(xr_ostimer_workq_stack_area, 2048);

typedef void (*csk_os_timer_handler_t)(void *arg);

/**
 * @brief Timer type definition
 *     - one shot timer: Timer will be in the dormant state after it expires.
 *     - periodic timer: Timer will auto-reload after it expires.
 */
typedef enum {
    CSK_OS_TIMER_ONESHOT = 0, /* one shot timer */
    CSK_OS_TIMER_PERIODIC = 1 /* periodic timer */
} csk_os_timer_type_t;

typedef struct {
	struct k_timer timer; /* Timer handle */
	struct k_work work;
	struct k_work_sync work_sync;
	k_timeout_t period;
	k_timeout_t duration;
	csk_os_timer_type_t timer_type;
	csk_os_timer_handler_t callback; /* Timer expire callback function */
	void *argument;			 /* Argument of timer expire callback function */
} z_timer_t;

void timer_entry(struct k_timer *timer)
{
	z_timer_t *timer_handle = k_timer_user_data_get(timer);
	k_work_submit_to_queue(&xr_ostimer_workq, &timer_handle->work);
}

static void timer_work_handler(struct k_work *work)
{
	z_timer_t *timer_handle = CONTAINER_OF(work, z_timer_t, work);
	timer_handle->callback(timer_handle->argument);
}

/**
 * @brief Create and initialize a timer object
 *
 * @note Creating a timer does not start the timer running. The OS_TimerStart()
 *       and OS_TimerChangePeriod() API functions can all be used to start the
 *       timer running.
 *
 * @param[in] timer Pointer to the timer object
 * @param[in] type Timer type
 * @param[in] cb Timer expire callback function
 * @param[in] arg Argument of Timer expire callback function
 * @param[in] period_ms Timer period in milliseconds
 * @retval OS_Status, OS_OK on success
 */
TimerHandle_t xTimerCreate(	const char * const pcTimerName,			/*lint !e971 Unqualified char types are allowed for strings and single characters only. */
                               const TickType_t xTimerPeriodInTicks,
                               const UBaseType_t uxAutoReload,
                               void * const pvTimerID,
                               TimerCallbackFunction_t pxCallbackFunction )
{
    ARG_UNUSED(pcTimerName);
	static bool is_init = false;
	if (!is_init) {
		k_work_queue_start(&xr_ostimer_workq, xr_ostimer_workq_stack_area,
				   K_THREAD_STACK_SIZEOF(xr_ostimer_workq_stack_area), 2, NULL);
		k_thread_name_set(&xr_ostimer_workq.thread, "xr_ostimer_workq");
		is_init = true;
	}

	z_timer_t *z_timer = csk_os_malloc(sizeof(z_timer_t));
	if (z_timer == NULL) {
		return NULL;
	}
    if (uxAutoReload == pdTRUE) {
        z_timer->timer_type = CSK_OS_TIMER_PERIODIC;
    } else {
        z_timer->timer_type = CSK_OS_TIMER_ONESHOT;
    }

	z_timer->callback = pxCallbackFunction;
	z_timer->argument = pvTimerID;
    k_timeout_t wait = Z_TIMEOUT_TICKS(xTimerPeriodInTicks);
    if (z_timer->timer_type == CSK_OS_TIMER_ONESHOT) {
		z_timer->duration = wait;
		z_timer->period = K_NO_WAIT;
	} else {
		z_timer->duration = wait;
		z_timer->period = wait;
	}

	k_timer_init(&z_timer->timer, &timer_entry, NULL);
	k_timer_user_data_set(&z_timer->timer, z_timer);
	k_work_init(&z_timer->work, timer_work_handler);

	return z_timer;
}

BaseType_t xTimerStart( TimerHandle_t xTimer, TickType_t xTicksToWait )
{
    ARG_UNUSED(xTicksToWait);
	z_timer_t *z_timer = xTimer;
	k_timer_start(&z_timer->timer, z_timer->duration, z_timer->period);
	return pdPASS;
}

BaseType_t xTimerDelete( TimerHandle_t xTimer, TickType_t xTicksToWait )
{
    ARG_UNUSED(xTicksToWait);
	z_timer_t *z_timer = xTimer;
	k_timer_stop(&z_timer->timer);
	k_work_cancel(&z_timer->work);
	k_work_cancel_sync(&z_timer->work, &z_timer->work_sync);
	csk_os_free(z_timer);
	return pdPASS;
}

BaseType_t xTimerChangePeriod( TimerHandle_t xTimer,
                               TickType_t xNewPeriod,
                               TickType_t xTicksToWait )
{
    z_timer_t *z_timer = xTimer;
    k_timeout_t wait = Z_TIMEOUT_TICKS(xTicksToWait);
    if (z_timer->timer_type == CSK_OS_TIMER_ONESHOT) {
        z_timer->duration = wait;
        z_timer->period = K_NO_WAIT;
    } else {
        z_timer->duration = wait;
        z_timer->period = wait;
    }
    k_timer_start(&z_timer->timer, z_timer->duration, z_timer->period);
    return pdPASS;
}

BaseType_t xTimerStop( TimerHandle_t xTimer, TickType_t xTicksToWait )
{
    ARG_UNUSED(xTicksToWait);
	z_timer_t *z_timer = xTimer;
	k_work_cancel(&z_timer->work);
	k_timer_stop(&z_timer->timer);
	return pdPASS;
}

BaseType_t xTimerReset( TimerHandle_t xTimer, TickType_t xTicksToWait )
{
    xTimerStop(xTimer, xTicksToWait);
    xTimerStart(xTimer, xTicksToWait);
    return pdPASS;
}
