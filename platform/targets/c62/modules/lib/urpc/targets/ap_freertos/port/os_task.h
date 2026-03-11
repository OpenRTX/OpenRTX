#pragma once

#include <FreeRTOS.h>
#include <task.h>

#define urpc_task_create_static(                                        \
	function, name, stack_depth, params, priority, stack_buf, task_buf) \
	xTaskCreateStatic(function, name, stack_depth, params, priority, stack_buf, task_buf)

#define urpc_task_delay(timeout) vTaskDelay(pdMS_TO_TICKS(timeout))

#define urpc_task_false pdFALSE
#define urpc_task_true pdTRUE
#define urpc_task_pass pdPASS
#define urpc_task_fail pdFAIL

#define urpc_task_get_current_handle() xTaskGetCurrentTaskHandle()

#define urpc_task_enter_critcal() taskENTER_CRITICAL()
#define urpc_task_exit_critcal() taskEXIT_CRITICAL()
