#include <zephyr/kernel.h>

#include "csk_os_memory.h"
#include "csk_os_semaphore.h"

typedef struct {
	struct k_sem *sem;
} z_sem_t;

SemaphoreHandle_t csk_os_semaphore_create(uint32_t init_count, uint32_t max_count)
{
	z_sem_t *z_sem = csk_os_malloc(sizeof(z_sem_t));
	if (z_sem == NULL) {
		return NULL;
	}

	z_sem->sem = csk_os_malloc(sizeof(struct k_sem));
	if (z_sem->sem == NULL) {
		goto __cleanup;
	}

	int ret = k_sem_init(z_sem->sem, init_count, max_count);
	if (ret != 0) {
		goto __cleanup;
	}
	return z_sem;

__cleanup:
	if (z_sem->sem) {
		csk_os_free(z_sem->sem);
		z_sem->sem = NULL;
	}
    csk_os_free(z_sem);
    z_sem = NULL;

	return NULL;
}

SemaphoreHandle_t xSemaphoreCreateMutex( void )
{
    SemaphoreHandle_t sem = csk_os_semaphore_create(0, 1);
    xSemaphoreGive(sem);
    return sem;
}

SemaphoreHandle_t xSemaphoreCreateBinary( void )
{
    return csk_os_semaphore_create(0, 1);
}

SemaphoreHandle_t xSemaphoreCreateCounting( UBaseType_t uxMaxCount, UBaseType_t uxInitialCount )
{
    return csk_os_semaphore_create(uxInitialCount, uxMaxCount);
}

void vSemaphoreDelete( SemaphoreHandle_t xSemaphore )
{
	z_sem_t *z_sem = xSemaphore;
	csk_os_free(z_sem->sem);
	z_sem->sem = NULL;
	csk_os_free(z_sem);
	z_sem = NULL;
}

BaseType_t xSemaphoreTake( SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait )
{
	z_sem_t *z_sem = xSemaphore;
    k_timeout_t timeout = Z_TIMEOUT_TICKS(xTicksToWait);
	int ret = k_sem_take(z_sem->sem, timeout);
	if (ret != 0) {
		return pdFAIL;
	}
	return pdPASS;
}

BaseType_t xSemaphoreGive( SemaphoreHandle_t xSemaphore )
{
	z_sem_t *z_sem = xSemaphore;
	if (z_sem->sem->count == z_sem->sem->limit) {
		return pdFAIL;
	}
	k_sem_give(z_sem->sem);
	return pdPASS;
}

UBaseType_t uxSemaphoreGetCount( SemaphoreHandle_t xSemaphore )
{
	z_sem_t *z_sem = xSemaphore;
	return (UBaseType_t) k_sem_count_get(z_sem->sem);
}
