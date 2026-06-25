/* Compile-pass fixture: an array of void * pointers has element type
 * void *, not void, so the mem_array_* guard accepts it. The real
 * __LOCATION__ lives in common.h, not mem.h, so a stand-in is defined
 * before the only include. */
#define __LOCATION__ "test"
#include "mem.h"

void trigger(void)
{
	void **vec = NULL;

	/* void * element: guard passes, sizeof(void *) sizes correctly */
	mem_array_alloc(&vec, 8);
}
