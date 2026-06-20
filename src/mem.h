#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <string.h>

#include "branch_hints.h"

/* Alignment for cache-line and AVX-512 friendliness */
#define MEM_ALIGNMENT  64
#define MEM_HEADER_SIZE 64

/* mem_obj_t occupies the first cache line of each allocation.
 * User data begins at base + MEM_HEADER_SIZE, also 64-byte aligned.
 *
 * Layout:
 *   |<-- MEM_HEADER_SIZE (64B) -->|<-- req bytes user data -->|
 *   [ mem_obj_t (40B) | pad 24B  ][ user ptr (64-byte aligned)]
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
 *   site: __LOCATION__ of the allocating call, for leak accounting
 *   ptr: pointer to user data region
 */
typedef struct mem_obj_t
{
	size_t size;
	size_t used;
	char **backtrace;
	const char *site;

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

/**
 * mem_array_count() - elements currently allocated in a managed array
 * @ptr: managed pointer, or NULL
 * @elem_size: size of one element
 *
 * Reads the live element count from the allocator header, the single
 * source of truth; callers keep no separate count.
 *
 * Return: used bytes / elem_size, or 0 when ptr is NULL
 */
static inline int mem_array_count(void *ptr, size_t elem_size)
{
	if (ptr == NULL)
		return 0;

	return (int)(mem_obj_from_ptr(ptr)->used / elem_size);
}

/**
 * mem_array_capacity() - elements a managed array holds before it must grow
 * @ptr: managed pointer, or NULL
 * @elem_size: size of one element
 *
 * Return: capacity bytes / elem_size, or 0 when ptr is NULL
 */
static inline int mem_array_capacity(void *ptr, size_t elem_size)
{
	if (ptr == NULL)
		return 0;

	return (int)(mem_obj_from_ptr(ptr)->size / elem_size);
}

/**
 * mem_elem_free_fn - per-element teardown callback for mem_array_resize
 * @elem: pointer to one array element to release sub-buffers from
 */
typedef void (*mem_elem_free_fn)(void *elem);

void mem_array_resize(void **arr, size_t elem_size, int new_count,
		      mem_elem_free_fn free_elem);

/**
 * mem_array_reserve - grow a managed array to hold at least @needed elements
 * @arr: address of the caller's array pointer
 * @elem_size: size of one element
 * @needed: minimum element count the array must hold
 * @initial_cap: capacity allocated when the array is empty
 */
void mem_array_reserve(void **arr, size_t elem_size, int needed,
		       int initial_cap);
void *mem_clone(void *ptr);
int mem_bcmp(void *p1, void *p2);
void mem_set(void *ptr, int c);

#endif /* MEM_H */
