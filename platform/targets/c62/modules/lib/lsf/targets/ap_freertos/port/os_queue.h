#pragma once

#include <FreeRTOS.h>
#include <queue.h>

#define lsf_queue_t QueueHandle_t

#define lsf_queue_create(length, item_size) xQueueCreate(length, item_size)
#define lsf_queue_delete(handle) vQueueDelete(handle)
#define lsf_queue_reset(handle) xQueueReset(handle)
#define lsf_queue_receive(handle, msg, delay) xQueueReceive(handle, msg, delay)
#define lsf_queue_send(handle, msg, delay) xQueueSend(handle, msg, delay)
#define lsf_queue_send_from_isr(handle, msg, delay) xQueueSendFromISR(handle, msg, delay)
#define lsf_queue_is_full_from_isr(handle) xQueueIsQueueFullFromISR(handle)
#define lsf_queue_messages_waiting_from_isr(handle) uxQueueMessagesWaitingFromISR(handle)
#define lsf_queue_send_to_back(handle, msg, delay) xQueueSendToBack(handle, msg, delay)
