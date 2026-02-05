#include <stdio.h>
#include <stdarg.h>
#include <gtk/gtk.h>

/* Stub implementations for xnec2c functions used by OpenGL modules */

enum
{
  PR_BUG,
  PR_ALERT,
  PR_CRIT,
  PR_ERR,
  PR_WARN,
  PR_NOTICE,
  PR_INFO,
  PR_DEBUG
};

int _xnec2c_printf(int level, const char *file, const char *func, int line, const char *format, ...)
{
  va_list args;
  int ret;
  const char *level_str;

  switch( level )
  {
    case PR_BUG:
      level_str = "BUG";
      break;
    case PR_ALERT:
      level_str = "ALERT";
      break;
    case PR_CRIT:
      level_str = "CRIT";
      break;
    case PR_ERR:
      level_str = "ERROR";
      break;
    case PR_WARN:
      level_str = "WARN";
      break;
    case PR_NOTICE:
      level_str = "NOTICE";
      break;
    case PR_INFO:
      level_str = "INFO";
      break;
    case PR_DEBUG:
      level_str = "DEBUG";
      break;
    default:
      level_str = "UNKNOWN";
      break;
  }

  fprintf(stderr, "[%s] %s:%s:%d: ", level_str, file, func, line);

  va_start(args, format);
  ret = vfprintf(stderr, format, args);
  va_end(args);

  return( ret );
}

void free_ptr(void **ptr)
{
  if( ptr && *ptr )
  {
    g_free(*ptr);
    *ptr = NULL;
  }
}

void Draw_Color_Legend_Overlay(cairo_t *cr)
{
  cairo_pattern_t *gradient;
  int i;

  gradient = cairo_pattern_create_linear(20, 20, 20, 200);
  for( i = 0; i <= 10; i++ )
  {
    double t = i / 10.0;
    double r = t;
    double g = 1.0 - t;
    double b = 0.5;
    cairo_pattern_add_color_stop_rgb(gradient, t, r, g, b);
  }

  cairo_set_source(cr, gradient);
  cairo_rectangle(cr, 20, 20, 40, 180);
  cairo_fill(cr);

  cairo_pattern_destroy(gradient);

  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 12);
  cairo_move_to(cr, 70, 30);
  cairo_show_text(cr, "Max");
  cairo_move_to(cr, 70, 195);
  cairo_show_text(cr, "Min");
}
