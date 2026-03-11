#pragma once

#include <FreeRTOS.h>
#include <semphr.h>

#define lsf_sem_t SemaphoreHandle_t

#define lsf_sem_create_mutex() xSemaphoreCreateMutex()
#define lsf_sem_create_counting(max_count, init_count) \
	xSemaphoreCreateCounting(max_count, init_count)
#define lsf_sem_take(handle, wait) xSemaphoreTake(handle, wait)
#define lsf_sem_give(handle) xSemaphoreGive(handle)
#define lsf_sem_take_from_isr(handle) xSemaphoreTakeFromISR(handle)
#define lsf_sem_get_count(handle) uxSemaphoreGetCount(handle)
