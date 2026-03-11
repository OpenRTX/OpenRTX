#pragma once

#include <FreeRTOS.h>
#include <semphr.h>

#define urpc_sem_t SemaphoreHandle_t

#define urpc_sem_create_mutex() xSemaphoreCreateMutex()
#define urpc_sem_create_counting(max_count, init_count) \
	xSemaphoreCreateCounting(max_count, init_count)
#define urpc_sem_take(max_count, init_count) xSemaphoreTake(max_count, init_count)
#define urpc_sem_give(handle) xSemaphoreGive(handle)
#define urpc_sem_take_from_isr(handle, woken) xSemaphoreTakeFromISR(handle, woken)
#define urpc_sem_give_from_isr(handle, woken) xSemaphoreGiveFromISR(handle, woken)
#define urpc_sem_get_count(handle) uxSemaphoreGetCount(handle)
