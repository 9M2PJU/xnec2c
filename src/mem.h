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

#define mem_alloc(ptr, size)   _mem_realloc(ptr, size, __LOCATION__)
#define mem_realloc(ptr, size) _mem_realloc(ptr, size, __LOCATION__)

void mem_backtrace(void *ptr);
void mem_obj_dump(void *ptr);
void mem_free(void **ptr);

#endif /* MEM_H */
