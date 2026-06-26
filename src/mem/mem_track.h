#ifndef MEM_TRACK_H
#define MEM_TRACK_H

#include <stddef.h>

/* Per-call-site live-allocation accounting for the managed allocator.
 * Keyed by the stable __LOCATION__ string pointer passed to mem_alloc. */
typedef struct
{
	const char    *site;
	size_t         live_bytes;
	size_t         prev_bytes;
	unsigned long  allocs;
	unsigned long  frees;
} mem_site_t;

void mem_track_add(const char *site, size_t bytes);
void mem_track_drop(const char *site, size_t bytes);
void mem_report(const char *tag);

#endif /* MEM_TRACK_H */
