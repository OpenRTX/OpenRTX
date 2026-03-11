#pragma once

#include <FreeRTOS.h>
#include <queue.h>

#define urpc_queue_t QueueHandle_t

#define urpc_queue_create(length, item_size) xQueueCreate(length, item_size)
#define urpc_queue_reset(handle) xQueueReset(handle)
#define urpc_queue_receive(handle, msg, delay) xQueueReceive(handle, msg, delay)
#define urpc_queue_send(handle, msg, delay) xQueueSend(handle, msg, delay)
#define urpc_queue_send_from_isr(handle, msg, delay) xQueueSendFromISR(handle, msg, delay)
#define urpc_queue_is_full_from_isr(handle) xQueueIsQueueFullFromISR(handle)
#define urpc_queue_messages_waiting_from_isr(handle) uxQueueMessagesWaitingFromISR(handle)
#define urpc_queue_send_to_back(handle, msg, delay) xQueueSendToBack(handle, msg, delay)
