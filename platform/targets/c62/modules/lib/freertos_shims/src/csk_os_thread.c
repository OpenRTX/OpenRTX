#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/device.h>

#include "csk_os_memory.h"
#include "csk_os_thread.h"


#define  THREAD_START_PRIO  (0)

#define Z_THREAD_STACK_SIZE_ADJUST(size)                                                           \
	(ROUND_UP((size), ARCH_STACK_PTR_ALIGN) + K_THREAD_STACK_RESERVED)

typedef struct {
	struct k_thread thread;
	k_thread_stack_t *stack;
} z_thread_t;

typedef struct thread_handle_delete_item {
	sys_snode_t node;
	z_thread_t *delete_handle;
} thread_handle_delete_item_t;

static sys_slist_t s_thread_delete_list;
static struct k_work_delayable s_thread_delete_work;

K_MUTEX_DEFINE(thread_delete_mutex);

static int thread_delete_list_free(void)
{
	thread_handle_delete_item_t *entry, *next;
	k_mutex_lock(&thread_delete_mutex, K_FOREVER);
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE (&s_thread_delete_list, entry, next, node) {
		z_thread_t *thread_handle = entry->delete_handle;
		if (thread_handle->stack) {
			csk_os_free(thread_handle->stack);
			thread_handle->stack = NULL;
		}
		csk_os_free(thread_handle);
		thread_handle = NULL;
		sys_slist_find_and_remove(&s_thread_delete_list, &entry->node);
		csk_os_free(entry);
		entry = NULL;
	}
	k_mutex_unlock(&thread_delete_mutex);
	return 0;
}

void csk_os_thread_yield(void)
{
	k_yield();
}

void csk_os_thread_sleep(uint32_t msec)
{
	k_msleep(msec);
}

TaskHandle_t xTaskGetCurrentTaskHandle( void )
{
	k_tid_t current_tid = k_current_get();
	z_thread_t *z_thread = CONTAINER_OF(current_tid, z_thread_t, thread);
	return (TaskHandle_t)z_thread;
}

static void zep_thread_entry(void *real_entry, void *real_arg, void *arg3)
{
	csk_thread_entry_t thread_handler = real_entry;
	thread_handler(real_arg);
}

BaseType_t xTaskCreate(	TaskFunction_t pxTaskCode,
							const char * const pcName,
							const configSTACK_DEPTH_TYPE usStackDepth,
							void * const pvParameters,
							UBaseType_t uxPriority,
							TaskHandle_t * const pxCreatedTask )
{
	z_thread_t *z_thread = csk_os_malloc(sizeof(z_thread_t));
	if (z_thread == NULL) {
		return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
	}

	int ret;
	z_thread->stack = csk_os_malloc(Z_THREAD_STACK_SIZE_ADJUST(usStackDepth));
	if (z_thread->stack == NULL) {
		ret = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
		goto __cleanup;
	}

    int z_priority = configMAX_PRIORITIES - uxPriority + THREAD_START_PRIO;
	k_tid_t tid = k_thread_create(&z_thread->thread, z_thread->stack,
				      Z_THREAD_STACK_SIZE_ADJUST(usStackDepth), &zep_thread_entry,
				      (void *)pxTaskCode, pvParameters, NULL, z_priority, 0, K_NO_WAIT);
	__ASSERT(tid == &z_thread->thread, "tid not valid");
	(void)tid;
	k_thread_name_set(&z_thread->thread, pcName);
	if (pxCreatedTask != NULL) {
		*pxCreatedTask = z_thread;
	}
	return 0;

__cleanup:
	if (z_thread->stack) {
		csk_os_free(z_thread->stack);
		z_thread->stack = NULL;
	}
    csk_os_free(z_thread);
	return ret;
}

TaskHandle_t xTaskCreateStatic( TaskFunction_t pvTaskCode,
								const char * const pcName,
								uint32_t ulStackDepth,
								void *pvParameters,
								UBaseType_t uxPriority,
								StackType_t * const puxStackBuffer,
								StaticTask_t * const pxTaskBuffer )
{
	ARG_UNUSED(pxTaskBuffer);
	z_thread_t *z_thread = csk_os_malloc(sizeof(z_thread_t));
	if (z_thread == NULL) {
		return NULL;
	}
	z_thread->stack = NULL;
	int z_priority = configMAX_PRIORITIES - uxPriority + THREAD_START_PRIO;
	k_tid_t tid = k_thread_create(&z_thread->thread, (k_thread_stack_t *) puxStackBuffer,
								  ulStackDepth, &zep_thread_entry,
								  (void *)pvTaskCode, pvParameters, NULL, z_priority, 0, K_NO_WAIT);

	__ASSERT(tid == &z_thread->thread, "tid not valid");
	(void)tid;
	k_thread_name_set(&z_thread->thread, pcName);
	return z_thread;
}

void vTaskDelay( TickType_t xTicksToDelay )
{
    k_sleep(Z_TIMEOUT_TICKS(xTicksToDelay));
}

void vTaskDelete( TaskHandle_t xTaskToDelete )
{
	bool is_delete_self;
	z_thread_t *z_thread = NULL;

	k_sched_lock();
	k_tid_t current_tid = k_current_get();
	if (xTaskToDelete == NULL) {
		z_thread = CONTAINER_OF(current_tid, z_thread_t, thread);
		is_delete_self = true;
	} else {
		z_thread = xTaskToDelete;
		if (k_current_get() == (&z_thread->thread)) {
			is_delete_self = true;
		} else {
			is_delete_self = false;
		}
	}

	if (is_delete_self) {
		k_work_cancel_delayable(&s_thread_delete_work);
		k_work_reschedule(&s_thread_delete_work, K_MSEC(50));
		k_sched_unlock();
		k_thread_abort(&z_thread->thread);
		return;
	}
	k_sched_unlock();
	k_thread_abort(&z_thread->thread);
	if (true == k_is_in_isr()) {
		k_thread_join(&z_thread->thread, K_NO_WAIT);
	} else {
		k_thread_join(&z_thread->thread, K_FOREVER);
	}

	csk_os_free(z_thread->stack);
	z_thread->stack = NULL;
	csk_os_free(z_thread);
}

void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority )
{
	z_thread_t *z_thread = xTask;
	int z_priority = configMAX_PRIORITIES - uxNewPriority + THREAD_START_PRIO;
	k_thread_priority_set(&z_thread->thread, z_priority);
}

static void thread_delete_handler(struct k_work *work)
{
	thread_delete_list_free();
}

static int os_thread_init(void)
{
	sys_slist_init(&s_thread_delete_list);
	k_work_init_delayable(&s_thread_delete_work, thread_delete_handler);
	return 0;
}

SYS_INIT(os_thread_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
