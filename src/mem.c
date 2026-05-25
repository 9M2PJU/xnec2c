#include "common.h"
#include "console.h"
#include "utils.h"

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
 * mem_obj_alloc() - allocate or reallocate aligned memory
 * @old_m: previous mem_obj_t or NULL for fresh allocation
 * @req: requested user-data size in bytes
 * @prev_used: bytes of old data to preserve (0 for fresh allocation)
 *
 * Uses posix_memalign with MEM_ALIGNMENT-byte alignment.
 * Total allocation is MEM_HEADER_SIZE + req.
 *
 * Return: pointer to new mem_obj_t, or NULL on failure
 */
static inline mem_obj_t *mem_obj_alloc(mem_obj_t *old_m, size_t req,
	size_t prev_used)
{
	void *base = NULL;
	mem_obj_t *m;
	int rc;

	rc = posix_memalign(&base, MEM_ALIGNMENT, MEM_HEADER_SIZE + req);
	if (unlikely(rc != 0 || base == NULL))
		return NULL;

	m = (mem_obj_t *)base;
	m->size = req;
	m->used = req;
	m->backtrace = NULL;
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

	return m;
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
		m = mem_obj_alloc(NULL, req, 0);
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

	/* Shrink or grow within existing capacity */
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

		m = mem_obj_alloc(old_m, req, prev_used);
		if (unlikely(m == NULL))
			goto alloc_fail;

		/* Free the old block */
		free(old_m);
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
 * mem_free() - release a managed memory buffer
 * @ptr: pointer to caller's void* (set to NULL on return)
 */
void mem_free(void **ptr)
{
	if (*ptr != NULL)
	{
		mem_obj_t *m = mem_obj_from_ptr(*ptr);

		if (m->backtrace != NULL)
			free(m->backtrace);

		/* Free from base of aligned allocation */
		free(m);
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

