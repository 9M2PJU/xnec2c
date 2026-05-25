#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <string.h>

/* Alignment for cache-line and AVX-512 friendliness */
#define MEM_ALIGNMENT  64
#define MEM_HEADER_SIZE 64

/* mem_obj_t occupies the first cache line of each allocation.
 * User data begins at base + MEM_HEADER_SIZE, also 64-byte aligned.
 *
 * Layout:
 *   |<-- MEM_HEADER_SIZE (64B) -->|<-- req bytes user data -->|
 *   [ mem_obj_t (32B) | pad 32B  ][ user ptr (64-byte aligned)]
 *   ^                              ^
 *   base (64-aligned)              m->ptr = base + MEM_HEADER_SIZE
 *
 * Fields:
 *   size: total allocated user-data capacity
 *   used: amount currently in use by caller
 *     - shrink: req <= used, no realloc
 *     - grow within capacity: used < req <= size, no realloc
 *     - grow beyond capacity: req > size, new posix_memalign
 *   backtrace: optional debug allocation trace
 *   ptr: pointer to user data region
 */
typedef struct mem_obj_t
{
	size_t size;
	size_t used;
	char **backtrace;

	void *ptr;
} mem_obj_t;

void *_mem_realloc(void **ptr, size_t req, char *str);
void _mem_validate_fail(void *ptr, void *base) __attribute__((noreturn));

/**
 * mem_obj_from_ptr() - recover mem_obj_t header from a user pointer
 * @ptr: user-facing pointer returned by _mem_realloc
 *
 * Single point of truth for the header-recovery arithmetic.
 * Calls _mem_validate_fail (which invokes BUG) on corruption.
 *
 * Return: pointer to the mem_obj_t header
 */
static inline mem_obj_t *mem_obj_from_ptr(void *ptr)
{
	mem_obj_t *m = (mem_obj_t *)((char *)ptr - MEM_HEADER_SIZE);

	if (unlikely(m->ptr != ptr))
	{
		_mem_validate_fail(ptr, m);
		return NULL;
	}

	return m;
}

/**
 * _mem_realloc_fast() - inline fast-path for same-size reallocations
 * @ptr: address of caller's pointer (may point to existing allocation)
 * @req: requested size in bytes
 * @str: location string for diagnostics
 *
 * Short-circuits when the existing allocation already satisfies the
 * request (m->used == req), avoiding the full realloc path.
 *
 * Return: user-facing pointer to the allocation
 */
static inline void *_mem_realloc_fast(void **ptr, size_t req, char *str)
{
	if (likely(*ptr != NULL))
	{
		mem_obj_t *m = mem_obj_from_ptr(*ptr);

		if (likely(m->used == req))
			return m->ptr;
	}

	return _mem_realloc(ptr, req, str);
}

#define mem_alloc(ptr, size)   _mem_realloc_fast(ptr, size, __LOCATION__)
#define mem_realloc(ptr, size) _mem_realloc_fast(ptr, size, __LOCATION__)

void mem_backtrace(void *ptr);
void mem_obj_dump(void *ptr);
void mem_free(void **ptr);

#endif /* MEM_H */
