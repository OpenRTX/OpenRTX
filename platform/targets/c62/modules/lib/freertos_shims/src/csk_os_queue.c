#include <zephyr/kernel.h>

#include "csk_os_memory.h"
#include "csk_os_queue.h"

typedef struct {
	struct k_msgq *msgq;
	char *msgq_buffer;
} z_msgq_t;

QueueHandle_t xQueueCreate( const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize)
{
	z_msgq_t *z_queue = csk_os_malloc(sizeof(z_msgq_t));
	if (z_queue == NULL) {
		return NULL;
	}

	z_queue->msgq = csk_os_malloc(sizeof(struct k_msgq));
	z_queue->msgq_buffer = csk_os_calloc(uxQueueLength, uxItemSize);
	if (z_queue->msgq == NULL || (z_queue->msgq_buffer == NULL)) {
		goto __cleanup;
	}
	k_msgq_init(z_queue->msgq, z_queue->msgq_buffer, uxItemSize, uxQueueLength);
	return z_queue;

__cleanup:
	if (z_queue->msgq) {
		csk_os_free(z_queue->msgq);
		z_queue->msgq = NULL;
	}
	if (z_queue->msgq_buffer) {
		csk_os_free(z_queue->msgq_buffer);
		z_queue->msgq_buffer = NULL;
	}
    csk_os_free(z_queue);
    z_queue = NULL;
	return NULL;
}

void vQueueDelete( QueueHandle_t xQueue )
{
	z_msgq_t *z_queue = xQueue;
	k_msgq_cleanup(z_queue->msgq);
	csk_os_free(z_queue->msgq);
	z_queue->msgq = NULL;
	csk_os_free(z_queue->msgq_buffer);
	z_queue->msgq_buffer = NULL;
	csk_os_free(z_queue);
}

BaseType_t xQueueSend( QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait)
{
	z_msgq_t *z_queue = xQueue;
	k_timeout_t timeout = Z_TIMEOUT_TICKS(xTicksToWait);
	int ret = k_msgq_put(z_queue->msgq, pvItemToQueue, timeout);
    if (ret != 0) {
        return errQUEUE_FULL;
    }
	return pdPASS;
}

BaseType_t xQueueReceive( QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait )
{
	z_msgq_t *z_queue = xQueue;
    k_timeout_t timeout = Z_TIMEOUT_TICKS(xTicksToWait);
	int ret = k_msgq_get(z_queue->msgq, pvBuffer, timeout);
    if (ret != 0) {
        return errQUEUE_EMPTY;
    }
    return pdPASS;
}

BaseType_t xQueueReset( QueueHandle_t xQueue )
{
    z_msgq_t *z_queue = xQueue;
    k_msgq_purge(z_queue->msgq);
    return pdPASS;
}

BaseType_t xQueueIsQueueFullFromISR( const QueueHandle_t pxQueue )
{
    z_msgq_t *z_queue = pxQueue;
    uint32_t ret = k_msgq_num_free_get(z_queue->msgq);
    if (ret == 0) {
        return pdTRUE;
    } else {
        return pdFALSE;
    }
}

UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t xQueue )
{
    z_msgq_t *z_queue = xQueue;
    return k_msgq_num_used_get(z_queue->msgq);
}
