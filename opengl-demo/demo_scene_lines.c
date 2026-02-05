#include "demo_scene_lines.h"
#include <stdlib.h>
#include <math.h>

static int current_mode = 0;
static color_point_t *vertices = NULL;
static int vertex_count = 0;
static unsigned int generation = 0;

static gl_vertex_attrib_t lines_attribs[] = {
  {"position", 3, offsetof(color_point_t, point)},
  {"color", 4, offsetof(color_point_t, color)}
};

static gboolean
lines_generate(gl_view_content_t *out)
{
  int i;

  if( !out )
    return( FALSE );

  if( vertices )
  {
    free(vertices);
    vertices = NULL;
  }

  if( current_mode == 0 )
  {
    vertex_count = 360 * 2;
    vertices = calloc(vertex_count, sizeof(color_point_t));
    if( !vertices )
      return( FALSE );

    int vidx = 0;
    for( i = 0; i < 360; i++ )
    {
      double angle1 = i * M_PI / 180.0;
      double angle2 = (i + 1) * M_PI / 180.0;

      float r = (float)i / 360.0f;
      float g = 1.0f - r;
      float b = 0.5f;

      vertices[vidx].point.x = cos(angle1);
      vertices[vidx].point.y = sin(angle1);
      vertices[vidx].point.z = 0.0f;
      vertices[vidx].color.r = r;
      vertices[vidx].color.g = g;
      vertices[vidx].color.b = b;
      vertices[vidx].color.a = 1.0f;
      vidx++;

      vertices[vidx].point.x = cos(angle2);
      vertices[vidx].point.y = sin(angle2);
      vertices[vidx].point.z = 0.0f;
      vertices[vidx].color.r = r;
      vertices[vidx].color.g = g;
      vertices[vidx].color.b = b;
      vertices[vidx].color.a = 1.0f;
      vidx++;
    }
  }
  else
  {
    vertex_count = 24;
    vertices = calloc(vertex_count, sizeof(color_point_t));
    if( !vertices )
      return( FALSE );

    int vidx = 0;

    #define SET_LINE(x1, y1, z1, x2, y2, z2, cr, cg, cb) \
      vertices[vidx].point.x = x1; vertices[vidx].point.y = y1; vertices[vidx].point.z = z1; \
      vertices[vidx].color.r = cr; vertices[vidx].color.g = cg; vertices[vidx].color.b = cb; vertices[vidx].color.a = 1.0f; \
      vidx++; \
      vertices[vidx].point.x = x2; vertices[vidx].point.y = y2; vertices[vidx].point.z = z2; \
      vertices[vidx].color.r = cr; vertices[vidx].color.g = cg; vertices[vidx].color.b = cb; vertices[vidx].color.a = 1.0f; \
      vidx++

    SET_LINE(-0.8, -0.8, 0.0, 0.8, -0.8, 0.0, 1.0, 0.0, 0.0);
    SET_LINE(0.8, -0.8, 0.0, 0.8, 0.8, 0.0, 1.0, 1.0, 0.0);
    SET_LINE(0.8, 0.8, 0.0, -0.8, 0.8, 0.0, 0.0, 1.0, 0.0);
    SET_LINE(-0.8, 0.8, 0.0, -0.8, -0.8, 0.0, 0.0, 1.0, 1.0);
    SET_LINE(-0.8, -0.8, 0.0, 0.0, 0.0, 0.8, 0.0, 0.0, 1.0);
    SET_LINE(0.8, -0.8, 0.0, 0.0, 0.0, 0.8, 1.0, 0.0, 1.0);
    SET_LINE(0.8, 0.8, 0.0, 0.0, 0.0, 0.8, 1.0, 0.5, 0.0);
    SET_LINE(-0.8, 0.8, 0.0, 0.0, 0.0, 0.8, 0.5, 0.0, 1.0);
    SET_LINE(-0.5, 0.0, 0.0, 0.5, 0.0, 0.0, 1.0, 1.0, 1.0);
    SET_LINE(0.0, -0.5, 0.0, 0.0, 0.5, 0.0, 1.0, 1.0, 1.0);
    SET_LINE(0.0, 0.0, -0.5, 0.0, 0.0, 0.5, 1.0, 1.0, 1.0);
    SET_LINE(-0.3, -0.3, 0.3, 0.3, 0.3, 0.3, 0.7, 0.7, 0.7);

    #undef SET_LINE
  }

  out->vertices = vertices;
  out->vertex_count = vertex_count;
  out->vertex_stride = sizeof(color_point_t);
  out->draw_mode = GL_LINES;
  out->r_max = 1.0f;
  out->zoom = 1.0f;
  out->model_scale = 1.0f;
  out->show_gradient = FALSE;
  out->generation = generation;

  return( TRUE );
}

static void
lines_cleanup(void)
{
  if( vertices )
  {
    free(vertices);
    vertices = NULL;
  }
  vertex_count = 0;
}

void
lines_scene_set_mode(int mode)
{
  if( mode != current_mode )
  {
    current_mode = mode;
    generation++;
  }
}

gl_view_config_t lines_view_config = {
  .vertex_shader_path = "resources/gl/color-vertex.glsl",
  .fragment_shader_path = "resources/gl/color-fragment.glsl",
  .attribs = lines_attribs,
  .attrib_count = 2,
  .vertex_stride = sizeof(color_point_t),
  .has_gradient = FALSE,
  .gradient_draw = NULL
};

gl_scene_provider_t lines_scene_provider = {
  .generate = lines_generate,
  .post_render = NULL,
  .cleanup = lines_cleanup
};
