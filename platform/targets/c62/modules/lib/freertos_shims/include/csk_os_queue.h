#pragma once

#include "os_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Queue object definition
 */

typedef void *QueueHandle_t;

/**
 * @brief Create and initialize a queue object
 * @param[in] queue Pointer to the queue object
 * @param[in] queue_len The maximum number of items that the queue can hold at
 *                     any one time.
 * @param[in] item_size The size, in bytes, of each data item that can be stored
 *                     in the queue.
 * @retval OS_Status, OS_OK on success
 */
QueueHandle_t xQueueCreate( const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize);
//int csk_os_queue_create(csk_os_queue_t *queue, uint32_t queue_len, uint32_t item_size);

/**
 * @brief Delete the queue object
 * @param[in] queue Pointer to the queue object
 * @retval int, 0 on success
 */
void vQueueDelete( QueueHandle_t xQueue );
//int csk_os_queue_delete(csk_os_queue_t queue);

/**
 * @brief Send (write) an item to the back of the queue
 * @param[in] queue Pointer to the queue object
 * @param[in] item Pointer to the data to be copied into the queue.
 *                 The size of each item the queue can hold is set when the
 *                 queue is created, and that many bytes will be copied from
 *                 item into the queue storage area.
 * @param[in] wait_ms The maximum amount of time the thread should remain in the
 *                   blocked state to wait for space to become available on the
 *                   queue, should the queue already be full.
 *                   XR_OS_WAIT_FOREVER for waiting forever, zero for no waiting.
 * @retval int, 0 on success
 */
BaseType_t xQueueSend( QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait);

#define xQueueSendToBack( xQueue, pvItemToQueue, xTicksToWait )     xQueueSend(xQueue, pvItemToQueue, xTicksToWait)

#define xQueueSendFromISR( xQueue, pvItemToQueue, pxHigherPriorityTaskWoken )   xQueueSend( xQueue, pvItemToQueue, 0)

/**
 * @brief Receive (read) an item from the queue
 * @param[in] queue Pointer to the queue object
 * @param[in] item Pointer to the memory into which the received data will be
 *                 copied. The length of the buffer must be at least equal to
 *                 the queue item size which is set when the queue is created.
 * @param[in] wait_ms The maximum amount of time the thread should remain in the
 *                   blocked state to wait for data to become available on the
 *                   queue, should the queue already be empty.
 *                   XR_OS_WAIT_FOREVER for waiting forever, zero for no waiting.
 * @retval int, 0 on success
 */
BaseType_t xQueueReceive( QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait );


BaseType_t xQueueReset( QueueHandle_t xQueue );

UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t xQueue );
#define uxQueueMessagesWaitingFromISR( xQueue ) uxQueueMessagesWaiting( xQueue )

BaseType_t xQueueIsQueueFullFromISR( const QueueHandle_t pxQueue );

#ifdef __cplusplus
}
#endif
