#include <string.h>
#include <zephyr/kernel.h>

void *csk_os_malloc(size_t size)
{
	return k_aligned_alloc(32, size);
}

void csk_os_free(void *ptr)
{
	k_free(ptr);
}

static inline bool size_mul_overflow(size_t a, size_t b, size_t *result)
{
	return __builtin_mul_overflow(a, b, result);
}

void *csk_os_calloc(size_t nmemb, size_t size)
{
	size_t bounds;
	if (size_mul_overflow(nmemb, size, &bounds)) {
		return NULL;
	}
	void *ptr = csk_os_malloc(nmemb * size);
	if (ptr != NULL) {
		(void)memset(ptr, 0, bounds);
	}
	return ptr;
}
