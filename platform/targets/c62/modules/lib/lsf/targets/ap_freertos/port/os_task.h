#pragma once

#include <FreeRTOS.h>
#include <task.h>

#define lsf_task_create_static(function, name, stack_depth, params, priority, stack_buf, task_buf) \
	xTaskCreateStatic(function, name, stack_depth, params, priority, stack_buf, task_buf)

#define lsf_task_delay(timeout) vTaskDelay(pdMS_TO_TICKS(timeout))

#define lsf_task_false pdFALSE
#define lsf_task_true pdTRUE
#define lsf_task_pass pdPASS
#define lsf_task_fail pdFAIL

#define lsf_task_get_current_handle() xTaskGetCurrentTaskHandle()

#define lsf_task_enter_critcal() taskENTER_CRITICAL()
#define lsf_task_exit_critcal() taskEXIT_CRITICAL()
