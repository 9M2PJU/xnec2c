#include "demo_scene_cylinder.h"
#include "opengl_cylinder.h"
#include <stdlib.h>
#include <math.h>

static int current_mode = 0;
static lit_cylinder_mesh_t mesh = {0};
static unsigned int generation = 0;

static gl_vertex_attrib_t cylinder_attribs[] = {
  {"position", 3, offsetof(lit_color_point_t, point)},
  {"normal", 3, offsetof(lit_color_point_t, normal)},
  {"color", 4, offsetof(lit_color_point_t, color)}
};

static gboolean
cylinder_generate(gl_view_content_t *out)
{
  int vertex_needed, vidx;
  double angle;
  int i;

  if( !out )
    return( FALSE );

  if( mesh.vertices )
  {
    free(mesh.vertices);
    mesh.vertices = NULL;
  }

  vertex_needed = 0;
  if( current_mode == 0 )
  {
    vertex_needed = opengl_cylinder_vertex_count(12) * 8;
  }
  else if( current_mode == 1 )
  {
    vertex_needed = opengl_cylinder_vertex_count(12) * 3;
  }

  if( vertex_needed == 0 )
  {
    out->vertices = NULL;
    out->vertex_count = 0;
    out->vertex_stride = sizeof(lit_color_point_t);
    out->draw_mode = GL_TRIANGLES;
    out->r_max = 1.0f;
    out->zoom = 1.0f;
    out->model_scale = 1.0f;
    out->show_gradient = FALSE;
    out->generation = generation;
    return( TRUE );
  }

  mesh.vertices = calloc(vertex_needed, sizeof(lit_color_point_t));
  if( !mesh.vertices )
    return( FALSE );

  vidx = 0;

  if( current_mode == 0 )
  {
    for( i = 0; i < 8; i++ )
    {
      angle = i * M_PI / 4.0;
      vidx = opengl_lit_cylinder_append(&mesh, vidx,
        0.0, 0.0, -0.5,
        cos(angle) * 0.8, sin(angle) * 0.8, 0.5,
        0.05, 12,
        (i % 2) ? 1.0f : 0.3f,
        (i / 2 % 2) ? 1.0f : 0.3f,
        (i / 4 % 2) ? 1.0f : 0.3f,
        1.0f);
    }
  }
  else
  {
    vidx = opengl_lit_cylinder_append(&mesh, vidx,
      -0.8, 0.0, 0.0, 0.8, 0.0, 0.0, 0.08, 12,
      1.0f, 0.2f, 0.2f, 1.0f);
    vidx = opengl_lit_cylinder_append(&mesh, vidx,
      0.0, -0.8, 0.0, 0.0, 0.8, 0.0, 0.08, 12,
      0.2f, 1.0f, 0.2f, 1.0f);
    vidx = opengl_lit_cylinder_append(&mesh, vidx,
      0.0, 0.0, -0.8, 0.0, 0.0, 0.8, 0.08, 12,
      0.2f, 0.2f, 1.0f, 1.0f);
  }

  mesh.vertex_count = vidx;

  out->vertices = mesh.vertices;
  out->vertex_count = mesh.vertex_count;
  out->vertex_stride = sizeof(lit_color_point_t);
  out->draw_mode = GL_TRIANGLES;
  out->r_max = 1.0f;
  out->zoom = 1.0f;
  out->model_scale = 1.0f;
  out->show_gradient = FALSE;
  out->generation = generation;

  return( TRUE );
}

static void
cylinder_cleanup(void)
{
  if( mesh.vertices )
  {
    free(mesh.vertices);
    mesh.vertices = NULL;
  }
  mesh.vertex_count = 0;
}

void
cylinder_scene_set_mode(int mode)
{
  if( mode != current_mode )
  {
    current_mode = mode;
    generation++;
  }
}

gl_view_config_t cylinder_view_config = {
  .vertex_shader_path = "resources/gl/lit-color-vertex.glsl",
  .fragment_shader_path = "resources/gl/lit-color-fragment.glsl",
  .attribs = cylinder_attribs,
  .attrib_count = 3,
  .vertex_stride = sizeof(lit_color_point_t),
  .has_gradient = FALSE,
  .gradient_draw = NULL
};

gl_scene_provider_t cylinder_scene_provider = {
  .generate = cylinder_generate,
  .post_render = NULL,
  .cleanup = cylinder_cleanup
};
