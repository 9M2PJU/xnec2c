#include "common.h"
#include "console.h"
#include "utils.h"

void mem_obj_dump(void *ptr)
{
	mem_obj_t *m = (mem_obj_t*)ptr;
	m--;

	pr_debug("mem_obj_t at %p:\n"
		"  size: %lu\n"
		"  used: %lu\n"
		"  addr: %p\n", (void *)m, (unsigned long)m->size,  (unsigned long)m->used, m->ptr);


}

void mem_backtrace(void *ptr)
{
	mem_obj_t *m = (mem_obj_t *)ptr;
	if (m == NULL)
	{
		print_backtrace("mem_backtrace(NULL)");

		return;
	}

	m--;
	pr_debug("mem_backtrace(%p):\n", ptr);
	if (m->backtrace == NULL)
		pr_debug("  m->backtrace is null, no backtrace data\n");
	else
		_print_backtrace(m->backtrace);

}

void _mem_realloc( void **ptr, size_t req, gchar *str )
{
  gchar mesg[MESG_SIZE];
  size_t prev_used;

  /* Get the original mem_obj_t object: */
  mem_obj_t *m = (mem_obj_t *)*ptr;

  if (m == NULL)
  {
	  prev_used = 0;
	  m = realloc( m, req+sizeof(mem_obj_t) );

	  if (m != NULL)
	  {
		  m->size = req;
		  m->used = req;
		  m->backtrace = NULL;
	  }
  }
  else
  {
	  /* Adjust pointer to struct location: */
	  m--;

	  if (m->backtrace != NULL)
	  {
		  free(m->backtrace);
		  m->backtrace = NULL;
	  }
	  prev_used = m->used;

	  if (req <= m->size)
	  {
		m->used = req;
	  }
	  else
	  {
		m = realloc( m, req+sizeof(mem_obj_t) );

		if (m != NULL)
		{
			m->size = req;
			m->used = req;
		}
	  }
  }

  if( m == NULL )
  {
    snprintf( mesg, sizeof(mesg),
        _("Memory re-allocation denied %s\n"), str );
    pr_err("%s: Memory requested %lu\n", mesg, (unsigned long)req);
    Stop( ERR_STOP, "%s", mesg );
	return;
  }

  m->ptr = (void*)(m+1);
  *ptr = m->ptr;

  /* Get a backtrace for the allocation.  Use mem_backtrace(ptr) to display it. */
  /* Debug only if you need it, this is very slow: */
  /*m->backtrace = _get_backtrace();*/

  if (m->used > prev_used)
	  memset(((uint8_t*)*ptr)+prev_used, 0x00, m->used - prev_used);

} /* End of _mem_realloc() */

/*------------------------------------------------------------------------*/

  void
free_ptr( void **ptr )
{
  if( *ptr != NULL )
  {
	mem_obj_t *m = (mem_obj_t *)*ptr;
	m--;
    free( m );
  }
  *ptr = NULL;

} /* End of free_ptr() */
