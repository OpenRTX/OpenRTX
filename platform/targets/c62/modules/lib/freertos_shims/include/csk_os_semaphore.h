#pragma once

#include "os_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *SemaphoreHandle_t;

/**
 * @brief Create and initialize a counting semaphore object
 * @param[in] sem Pointer to the semaphore object
 * @param[in] init_count The count value assigned to the semaphore when it is
 *                      created.
 * @param[in] max_count The maximum count value that can be reached. When the
 *                     semaphore reaches this value it can no longer be
 *                     released.
 * @retval int, 0 on success
 */
SemaphoreHandle_t xSemaphoreCreateBinary( void );
SemaphoreHandle_t xSemaphoreCreateCounting( UBaseType_t uxMaxCount, UBaseType_t uxInitialCount );

/**
 * @brief Delete the semaphore object
 * @param[in] sem Pointer to the semaphore object
 * @retval int, 0 on success
 */
void vSemaphoreDelete( SemaphoreHandle_t xSemaphore );

/**
 * @brief Wait until the semaphore object becomes available
 * @param[in] sem Pointer to the semaphore object
 * @param[in] wait_ms The maximum amount of time (in millisecond) the thread
 *                   should remain in the blocked state to wait for the
 *                   semaphore to become available.
 *                   XR_OS_WAIT_FOREVER for waiting forever, zero for no waiting.
 * @retval int, 0 on success
 */
BaseType_t xSemaphoreTake( SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait );

#define xSemaphoreTakeFromISR( xSemaphore, pxHigherPriorityTaskWoken )  xSemaphoreTake(xSemaphore, 0)

SemaphoreHandle_t xSemaphoreCreateMutex( void );
/**
 * @brief Release the semaphore object
 * @param[in] sem Pointer to the semaphore object
 * @retval int, 0 on success
 */
BaseType_t xSemaphoreGive( SemaphoreHandle_t xSemaphore );
#define xSemaphoreGiveFromISR(xSemaphore, pxHigherPriorityTaskWoken)    xSemaphoreGive(xSemaphore)

UBaseType_t uxSemaphoreGetCount( SemaphoreHandle_t xSemaphore );

#ifdef __cplusplus
}
#endif
