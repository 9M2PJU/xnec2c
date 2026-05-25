#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <glib/gtypes.h>

/* xnec2c uses mem_realloc() very often to resize buffers in the code.
 * There are cases where uninitialized memory buffers can lead to incorrect behavior
 * but unfortunately the libc realloc() call doesn't initialize the reallocated memory.
 *
 * Since there is not a portable way to discover the amount of memory allocated by the
 * previous realloc/malloc() call we cannot call memset() on the extended portion.
 *
 * To solve this mem_obj_t stores a couple sizes and a pointer:
 *    1. size: the total allocated size requested by the caller
 *             Note that the actual allocated size is greater by sizeof(mem_obj_t)
 *    2. used: The amount actually used by the caller
 *         a. If mem_realloc() is called with a smaller amount of memory requested
 *            then it will shrink the `used` size without reallocating.
 *
 *         b. If mem_realloc() is called requesting more than `used` but less than `size`
 *            then it is grown without reallocating.
 *
 *         c. If mem_realloc() is called requesting more than `size` then realloc() is
 *            called and both `size` and `used` are updated.
 *
 *    3. ptr: a pointer to the memory used by the caller
 *
 * A mem_obj_t object is allocated in excess of the structure size by the amount
 * of memory requested by the caller.  When ptr_free or mem_realloc are called
 * we use decrement by sizeof(mem_obj_t) to access the original
 * structure pointer.
 */
typedef struct mem_obj_t
{
	size_t size;
	size_t used;
	char **backtrace;

	void *ptr;
} mem_obj_t;

void _mem_realloc(void **ptr, size_t req, gchar *str);

#define mem_alloc(ptr, size)   _mem_realloc(ptr, size, __LOCATION__)
#define mem_realloc(ptr, size) _mem_realloc(ptr, size, __LOCATION__)

void mem_backtrace(void *ptr);
void mem_obj_dump(void *ptr);
void mem_free(void **ptr);

#endif /* MEM_H */
