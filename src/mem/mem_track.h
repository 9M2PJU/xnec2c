#ifndef MEM_TRACK_H
#define MEM_TRACK_H

#include <stdint.h>

#include "mem.h"

/* Live-allocation registry for the managed allocator. Every managed block
 * links into one intrusive doubly linked list through its header; the list
 * is the single source of truth for the live set, enumerated at report
 * time. A monotonic serial stamped at birth names each block across the
 * relocation a grow-beyond reallocation performs. */
void mem_track_register(mem_obj_t *m);
void mem_track_unregister(mem_obj_t *m);
uint64_t mem_track_next_serial(void);
void mem_report(const char *tag);

#endif /* MEM_TRACK_H */
