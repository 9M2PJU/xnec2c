#include "common.h"
#include "console.h"
#include "utils.h"
#include "mem_track.h"

_Static_assert(sizeof(mem_obj_t) <= MEM_HEADER_SIZE,
	"mem_obj_t exceeds MEM_HEADER_SIZE");

/**
 * _mem_validate_fail() - report corrupted mem_obj_t header via BUG
 * @ptr: user-facing pointer that failed validation
 * @base: computed base address of the mem_obj_t
 *
 * The header lives at (ptr - MEM_HEADER_SIZE).  Validates that
 * the stored ptr field matches the caller's pointer.
 *
 */
void _mem_validate_fail(void *ptr, void *base)
{
	mem_obj_t *m = (mem_obj_t *)base;

	BUG("mem_obj_from_ptr: ptr %p does not match m->ptr %p (base %p)\n",
		ptr, m->ptr, base);
	abort();
}

/**
 * mem_obj_alloc() - allocate aligned memory, optionally copying from a source
 * @old_m: read-only source for data copy, or NULL for fresh allocation.
 *         Always allocates a new block; never modifies or frees old_m.
 * @req: requested user-data size in bytes
 * @prev_used: bytes of old data to preserve (0 for fresh allocation)
 *
 * Uses posix_memalign with MEM_ALIGNMENT-byte alignment.
 * Total allocation is MEM_HEADER_SIZE + req.
 *
 * Return: pointer to new mem_obj_t, or NULL on failure
 */
static inline mem_obj_t *mem_obj_alloc(const mem_obj_t *old_m, size_t req,
	size_t prev_used, const char *site)
{
	void *base = NULL;
	mem_obj_t *m;
	int rc;

	/* Minimum 1-byte user region so m->ptr stays within the allocation */
	if (req == 0)
		req = 1;

	rc = posix_memalign(&base, MEM_ALIGNMENT, MEM_HEADER_SIZE + req);
	if (unlikely(rc != 0 || base == NULL))
		return NULL;

	m = (mem_obj_t *)base;
	m->size = req;
	m->used = req;
	/* Single relocation-persistence point: a grow-beyond reallocation
	 * returns a fresh block, so every header field that must outlive the
	 * predecessor is carried from it here. A fresh allocation has no
	 * predecessor and starts at the scalar/byte default; the array layer
	 * stamps the element size at birth. */
	m->array_elem_size = (old_m != NULL) ? old_m->array_elem_size : 0;
	m->backtrace = NULL;
	/* Tag for leak accounting only while reporting is enabled.  An
	 * untracked block carries a NULL site so its release skips the
	 * accounting path, keeping add and drop balanced across the
	 * monotonic flag edge without a mutex on the disabled fast path. */
	m->site = rc_config.mem_report_enabled ? site : NULL;
	m->ptr = (char *)base + MEM_HEADER_SIZE;

	/* Preserve old data when reallocating */
	if (old_m != NULL && prev_used > 0)
	{
		size_t copy_size = (prev_used < req) ? prev_used : req;

		memcpy(m->ptr, old_m->ptr, copy_size);
	}

	/* Zero newly extended region */
	if (req > prev_used)
		memset((char *)m->ptr + prev_used, 0, req - prev_used);

	if (m->site != NULL)
		mem_track_add(m->site, m->size);

	return m;
}

/**
 * mem_obj_free() - release a managed block and its backtrace
 * @m: header to release
 *
 * Single release point: drops the per-site live total for tracked
 * blocks, then frees the optional backtrace and the aligned base block.
 */
static inline void mem_obj_free(mem_obj_t *m)
{
	if (m->site != NULL)
		mem_track_drop(m->site, m->size);

	if (m->backtrace != NULL)
		free(m->backtrace);

	free(m);
}

/**
 * _mem_realloc() - allocate or resize a managed memory buffer
 * @ptr: pointer to caller's void* (set to new user pointer on return)
 * @req: requested size in bytes
 * @str: caller location string from __LOCATION__
 *
 * On fresh allocation (*ptr == NULL), allocates aligned memory.
 * On reallocation, reuses the buffer when capacity suffices,
 * otherwise allocates new aligned memory and copies data.
 *
 * Emits pr_crit on allocation failure.
 *
 * Return: the new user-data pointer (also stored in *ptr)
 */
void *_mem_realloc(void **ptr, size_t req, char *str)
{
	mem_obj_t *m;
	size_t prev_used;

	if (likely(*ptr == NULL))
	{
		/* Fresh allocation */
		m = mem_obj_alloc(NULL, req, 0, str);
		if (unlikely(m == NULL))
			goto alloc_fail;

		*ptr = m->ptr;
		return m->ptr;
	}

	m = mem_obj_from_ptr(*ptr);

	if (m->backtrace != NULL)
	{
		free(m->backtrace);
		m->backtrace = NULL;
	}

	prev_used = m->used;

	/* Shrink or grow within existing capacity.
	 * On shrink (req < prev_used) the abandoned tail is not zeroed;
	 * grow-within-capacity re-zeroes the exposed region below. */
	if (req <= m->size)
	{
		m->used = req;

		/* Zero newly exposed region on grow-within-capacity */
		if (req > prev_used)
			memset((char *)m->ptr + prev_used, 0, req - prev_used);

		*ptr = m->ptr;
		return m->ptr;
	}

	/* Must reallocate: request exceeds capacity */
	{
		mem_obj_t *old_m = m;

		m = mem_obj_alloc(old_m, req, prev_used, str);
		if (unlikely(m == NULL))
			goto alloc_fail;

		/* Free the old block */
		mem_obj_free(old_m);
	}

	/* Debug only if you need it, this is very slow: */
	/*m->backtrace = _get_backtrace();*/

	*ptr = m->ptr;
	return m->ptr;

alloc_fail:
	pr_crit("Memory allocation denied %s: requested %lu bytes\n",
		str, (unsigned long)req);
	abort();
}

/**
 * _mem_free() - release a managed memory buffer
 * @ptr: pointer to caller's void* (set to NULL on return)
 *
 * The public mem_free macro strips the pointer cast before calling here.
 */
void _mem_free(void **ptr)
{
	if (*ptr != NULL)
	{
		mem_obj_t *m = mem_obj_from_ptr(*ptr);

		mem_obj_free(m);
	}

	*ptr = NULL;
}

/**
 * mem_obj_dump() - print debug info for a managed buffer
 * @ptr: user-facing pointer
 */
void mem_obj_dump(void *ptr)
{
	mem_obj_t *m = mem_obj_from_ptr(ptr);

	pr_debug("mem_obj_t at %p: size=%lu used=%lu addr=%p\n",
		(void *)m, (unsigned long)m->size,
		(unsigned long)m->used, m->ptr);
}

/**
 * mem_backtrace() - display allocation backtrace for a managed buffer
 * @ptr: user-facing pointer
 */
void mem_backtrace(void *ptr)
{
	mem_obj_t *m;

	if (ptr == NULL)
	{
		print_backtrace("mem_backtrace(NULL)");
		return;
	}

	m = mem_obj_from_ptr(ptr);

	pr_debug("mem_backtrace(%p):\n", ptr);

	if (m->backtrace == NULL)
		pr_debug("  m->backtrace is null, no backtrace data\n");
	else
		_print_backtrace(m->backtrace);
}

/**
 * mem_clone() - duplicate a managed memory buffer
 * @ptr: user-facing pointer to an existing managed allocation
 *
 * Allocates a new managed buffer of the same used size and copies
 * the contents of the source buffer into it.
 *
 * Return: user-facing pointer to the new allocation
 */
void *mem_clone(void *ptr)
{
	mem_obj_t *src = mem_obj_from_ptr(ptr);
	mem_obj_t *dst;

	/* mem_obj_alloc always creates a new block; src is read-only copy source */
	dst = mem_obj_alloc(src, src->used, src->used, __LOCATION__);
	if (unlikely(dst == NULL))
	{
		pr_crit("mem_clone: allocation failed for %lu bytes\n",
			(unsigned long)src->used);
		abort();
	}

	return dst->ptr;
}

/**
 * mem_bcmp() - compare contents of two managed buffers
 * @p1: user-facing pointer to first managed allocation
 * @p2: user-facing pointer to second managed allocation
 *
 * Compares the used regions of two managed buffers. Buffers with
 * different used sizes are considered unequal.
 *
 * Return: 0 if equal, non-zero otherwise (memcmp semantics)
 */
int mem_bcmp(void *p1, void *p2)
{
	mem_obj_t *m1 = mem_obj_from_ptr(p1);
	mem_obj_t *m2 = mem_obj_from_ptr(p2);

	if (m1->used != m2->used)
		return (m1->used > m2->used) ? 1 : -1;

	return memcmp(m1->ptr, m2->ptr, m1->used);
}

/**
 * mem_set() - fill a managed buffer with a byte value
 * @ptr: user-facing pointer to a managed allocation
 * @c: byte value to fill with
 *
 * Sets every byte in the used region of the managed buffer to c.
 */
void mem_set(void *ptr, int c)
{
	mem_obj_t *m = mem_obj_from_ptr(ptr);

	memset(m->ptr, c, m->used);
}

/**
 * mem_array_guard() - reject an array verb on a non-array or mismatched block
 * @m: header recovered from an existing managed block
 * @elem_size: element size derived from the caller's typed pointer
 *
 * A live array block carries a non-zero array_elem_size equal to its
 * element size. A zero marker (scalar or byte block) or a size mismatch is
 * a programmer error, surfaced like the header-corruption path rather than
 * tolerated.
 */
static void mem_array_guard(const mem_obj_t *m, size_t elem_size)
{
	if (unlikely(m->array_elem_size == 0 || m->array_elem_size != elem_size))
	{
		BUG("mem_array verb: element size %lu does not match header %lu\n",
			(unsigned long)elem_size, (unsigned long)m->array_elem_size);
		abort();
	}
}

/**
 * _mem_array_alloc() - allocate a managed typed array
 * @pp: address of the caller's array pointer
 * @elem_size: element size folded from the typed pointer at the macro
 * @n: element count
 * @site: caller location from __LOCATION__
 *
 * Sizes the request at n * elem_size and stamps the element size into the
 * header so later array verbs can validate and derive count and capacity.
 *
 * Return: the user-data pointer (also stored in *pp)
 */
void *_mem_array_alloc(void **pp, size_t elem_size, int n, char *site)
{
	if (unlikely(*pp != NULL))
		mem_array_guard(mem_obj_from_ptr(*pp), elem_size);

	void *p = _mem_realloc_fast(pp, (size_t)n * elem_size, site);

	mem_obj_from_ptr(p)->array_elem_size = elem_size;

	return p;
}

/**
 * _mem_array_realloc() - resize a managed typed array
 * @pp: address of the caller's array pointer
 * @elem_size: element size folded from the typed pointer at the macro
 * @n: new element count
 * @site: caller location from __LOCATION__
 *
 * Validates an existing block against the array type guard, then resizes
 * to n * elem_size and stamps the element size, establishing it when the
 * array is born from a NULL pointer; persistence across a grow-beyond
 * relocation is handled by the allocator's carry-forward point.
 *
 * Return: the user-data pointer (also stored in *pp)
 */
void *_mem_array_realloc(void **pp, size_t elem_size, int n, char *site)
{
	if (likely(*pp != NULL))
		mem_array_guard(mem_obj_from_ptr(*pp), elem_size);

	void *p = _mem_realloc_fast(pp, (size_t)n * elem_size, site);

	mem_obj_from_ptr(p)->array_elem_size = elem_size;

	return p;
}

/**
 * _mem_array_free() - release a managed typed array
 * @pp: address of the caller's array pointer (set to NULL on return)
 * @elem_size: element size folded from the typed pointer at the macro
 *
 * Validates the block against the array type guard before release; the
 * drop accounting reads the site stored at allocation, so no site is
 * passed here.
 */
void _mem_array_free(void **pp, size_t elem_size)
{
	if (*pp != NULL)
	{
		mem_obj_t *m = mem_obj_from_ptr(*pp);

		mem_array_guard(m, elem_size);
		mem_obj_free(m);
	}

	*pp = NULL;
}

/**
 * _mem_array_resize() - resize an array of structs, freeing dropped elements
 * @arr: address of the caller's array pointer
 * @elem_size: size of one array element
 * @new_count: new element count (caller guarantees >= 1)
 * @free_elem: per-element teardown for the shrink tail
 * @site: caller location from __LOCATION__, forwarded to the allocator
 *
 * Reads the prior element count from the allocator header, releases
 * sub-buffers of the dropped tail [new_count, old_count), then reallocates
 * the outer array in place. Surviving entries [0, new_count) keep their
 * pointers for reuse; any grown region is zeroed by the allocator.
 */
void _mem_array_resize(void **arr, size_t elem_size, int new_count,
		      mem_elem_free_fn free_elem, char *site)
{
	int old_count = mem_array_count(*arr, elem_size);

	for (int i = new_count; i < old_count; i++)
		free_elem((char *)*arr + (size_t)i * elem_size);

	/* Reuse the typed realloc primitive so the element-size stamp and the
	 * type guard apply here exactly as on any other typed growth. */
	_mem_array_realloc(arr, elem_size, new_count, site);
}

/**
 * _mem_array_reserve() - grow a managed array to hold at least @needed elements
 * @arr: address of the caller's array pointer
 * @elem_size: size of one element
 * @needed: minimum element count the array must hold
 * @initial_cap: capacity allocated when the array is empty
 * @site: caller location from __LOCATION__, forwarded to the allocator
 *
 * Doubles the current capacity until it covers @needed, reusing the block
 * in place; an array already large enough is left untouched. The caller
 * tracks its own fill count.
 */
void _mem_array_reserve(void **arr, size_t elem_size, int needed,
		       int initial_cap, char *site)
{
	int cap = mem_array_capacity(*arr, elem_size);

	if (cap >= needed)
		return;

	int new_cap = cap ? cap : initial_cap;
	while (new_cap < needed)
		new_cap *= 2;

	/* Reuse the typed realloc primitive so the element-size stamp and the
	 * type guard apply here exactly as on any other typed growth. */
	_mem_array_realloc(arr, elem_size, new_cap, site);
}

