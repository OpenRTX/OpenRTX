#pragma once

#include "os_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Timer expire callback function definition */
//typedef void *csk_os_timer_t;

typedef void *TimerHandle_t;
typedef void (*TimerCallbackFunction_t)( TimerHandle_t xTimer );

/**
 * @brief Create and initialize a timer object
 *
 * @note Creating a timer does not start the timer running. The csk_os_timer_start()
 *       and XR_OS_TimerChangePeriod() API functions can all be used to start the
 *       timer running.
 *
 * @param[in] timer Pointer to the timer object
 * @param[in] type Timer type
 * @param[in] cb Timer expire callback function
 * @param[in] arg Argument of Timer expire callback function
 * @param[in] period_ms Timer period in milliseconds
 * @retval int, XR_OS_OK on success
 */
TimerHandle_t xTimerCreate(	const char * const pcTimerName,
                               const TickType_t xTimerPeriodInTicks,
                               const UBaseType_t uxAutoReload,
                               void * const pvTimerID,
                               TimerCallbackFunction_t pxCallbackFunction );
/**
 * @brief Delete the timer object
 * @param[in] timer Pointer to the timer object
 * @retval int, XR_OS_OK on success
 */
BaseType_t xTimerDelete( TimerHandle_t xTimer, TickType_t xTicksToWait );

/**
 * @brief Start a timer running.
 * @note If the timer is already running, this function will re-start the timer.
 * @param[in] timer Pointer to the timer object
 * @retval int, XR_OS_OK on success
 */
BaseType_t xTimerStart( TimerHandle_t xTimer, TickType_t xTicksToWait );

/**
 * @brief Change the period of a timer
 *
 * If OS_TimerChangePeriod() is used to change the period of a timer that is
 * already running, then the timer will use the new period value to recalculate
 * its expiry time. The recalculated expiry time will then be relative to when
 * OS_TimerChangePeriod() was called, and not relative to when the timer was
 * originally started.

 * If OS_TimerChangePeriod() is used to change the period of a timer that is
 * not already running, then the timer will use the new period value to
 * calculate an expiry time, and the timer will start running.
 *
 * @param[in] timer Pointer to the timer object
 * @retval OS_Status, OS_OK on success
 */
BaseType_t xTimerChangePeriod( TimerHandle_t xTimer,
                               TickType_t xNewPeriod,
                               TickType_t xTicksToWait );

/**
 * @brief Stop a timer running.
 * @param[in] timer Pointer to the timer object
 * @retval int, XR_OS_OK on success
 */
BaseType_t xTimerStop( TimerHandle_t xTimer, TickType_t xTicksToWait );


BaseType_t xTimerReset( TimerHandle_t xTimer, TickType_t xTicksToWait );

#ifdef __cplusplus
}
#endif
