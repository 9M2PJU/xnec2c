/* Compile-fail fixture: a void element type must trip the mem_array_*
 * element-type guard. The real __LOCATION__ lives in common.h, not mem.h,
 * so a stand-in is defined before the only include. */
#define __LOCATION__ "test"
#include "mem.h"

void trigger(void)
{
	void *buf = NULL;

	/* void element: MEM_ARRAY_TYPED rejects it at compile time */
	mem_array_alloc(&buf, 8);
}
