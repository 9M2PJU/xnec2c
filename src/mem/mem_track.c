#include <stdint.h>
#include <pthread.h>

#include "common.h"
#include "console.h"
#include "mem_track.h"

/* Open-addressed site table.  The location-string pointer is the key;
 * a NULL site marks an empty slot.  One mutex serialises access because
 * the parent allocates from both the GTK main thread and the
 * frequency-loop thread. */
#define MEM_TRACK_SLOTS 8192u

static mem_site_t mem_sites[MEM_TRACK_SLOTS];
static pthread_mutex_t mem_track_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * mem_track_slot() - locate or insert the record for a call site
 * @site: stable location-string pointer (non-NULL key)
 *
 * Caller must hold mem_track_mutex.  Raises BUG on table overflow.
 *
 * Return: record pointer, or NULL when the table is exhausted
 */
static mem_site_t *mem_track_slot(const char *site)
{
	uintptr_t h = (uintptr_t)site;
	unsigned idx = (unsigned)((h >> 4) ^ (h >> 16)) & (MEM_TRACK_SLOTS - 1);
	unsigned probe;

	for (probe = 0; probe < MEM_TRACK_SLOTS; probe++)
	{
		mem_site_t *rec = &mem_sites[(idx + probe) & (MEM_TRACK_SLOTS - 1)];

		if (rec->site == site)
			return rec;

		if (rec->site == NULL)
		{
			rec->site = site;
			return rec;
		}
	}

	BUG("mem_track: site table overflow (%u slots)\n", MEM_TRACK_SLOTS);
	return NULL;
}

/**
 * mem_track_add() - record a managed allocation against its call site
 * @site: location-string pointer of the allocating call
 * @bytes: capacity of the new block
 */
void mem_track_add(const char *site, size_t bytes)
{
	mem_site_t *rec;

	pthread_mutex_lock(&mem_track_mutex);

	rec = mem_track_slot(site);
	if (rec != NULL)
	{
		rec->live_bytes += bytes;
		rec->allocs++;
	}

	pthread_mutex_unlock(&mem_track_mutex);
}

/**
 * mem_track_drop() - record a managed release against its call site
 * @site: location-string pointer recorded at allocation time
 * @bytes: capacity of the released block
 */
void mem_track_drop(const char *site, size_t bytes)
{
	mem_site_t *rec;

	pthread_mutex_lock(&mem_track_mutex);

	rec = mem_track_slot(site);
	if (rec != NULL)
	{
		rec->live_bytes -= bytes;
		rec->frees++;
	}

	pthread_mutex_unlock(&mem_track_mutex);
}

/**
 * mem_report() - emit live-allocation totals per call site
 * @tag: label identifying the report point
 *
 * Emits one line per site holding live bytes, reporting the delta
 * against the previous report so a repeating positive delta marks a
 * surviving leak.  Suppressed in forked compute children, which do not
 * hold the parent-side per-step buffers.
 */
void mem_report(const char *tag)
{
	unsigned i;

	if (CHILD)
		return;

	pthread_mutex_lock(&mem_track_mutex);

	for (i = 0; i < MEM_TRACK_SLOTS; i++)
	{
		mem_site_t *rec = &mem_sites[i];

		if (rec->site == NULL)
			continue;

		if (rec->live_bytes != 0)
		{
			long long delta_bytes = (long long)rec->live_bytes
				- (long long)rec->prev_bytes;
			/* Blocks allocated at this site and not yet freed,
			 * derived from the two authoritative monotonic counters
			 * rather than a mirrored tally.  A signed format surfaces
			 * underflow from any unbalanced release. */
			long long live_blocks = (long long)rec->allocs
				- (long long)rec->frees;

			/* Fields run raw-to-calculated, left to right: the
			 * stored counters first, then the values derived at
			 * print, with delta_bytes last as the leak-scan column. */
			pr_info("mem-report %s: %s block_allocs=%lu block_frees=%lu live_bytes=%lu live_blocks=%+lld delta_bytes=%+lld\n",
				tag, rec->site, rec->allocs, rec->frees,
				(unsigned long)rec->live_bytes, live_blocks, delta_bytes);
		}

		rec->prev_bytes = rec->live_bytes;
	}

	pthread_mutex_unlock(&mem_track_mutex);
}
