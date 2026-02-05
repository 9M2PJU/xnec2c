#include "demo_scene_triangle.h"
#include <stdlib.h>
#include <math.h>

static int current_mode = 0;
static gboolean show_gradient = FALSE;
static color_point_t *vertices = NULL;
static int vertex_count = 0;
static unsigned int generation = 0;

static gl_vertex_attrib_t triangle_attribs[] = {
  {"position", 3, offsetof(color_point_t, point)},
  {"color", 4, offsetof(color_point_t, color)}
};

static void
draw_gradient_overlay(cairo_t *cr)
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

static gboolean
triangle_generate(gl_view_content_t *out)
{
  int i, j;
  double theta, phi;
  int rings, sectors;

  if( !out )
    return( FALSE );

  if( vertices )
  {
    free(vertices);
    vertices = NULL;
  }

  if( current_mode == 0 )
  {
    rings = 20;
    sectors = 40;
    vertex_count = rings * sectors * 6;

    vertices = calloc(vertex_count, sizeof(color_point_t));
    if( !vertices )
      return( FALSE );

    int vidx = 0;
    for( i = 0; i < rings; i++ )
    {
      for( j = 0; j < sectors; j++ )
      {
        double t1 = (double)i / rings;
        double t2 = (double)(i + 1) / rings;
        double p1 = (double)j / sectors;
        double p2 = (double)(j + 1) / sectors;

        theta = t1 * M_PI;
        phi = p1 * 2.0 * M_PI;
        double x1 = sin(theta) * cos(phi);
        double y1 = sin(theta) * sin(phi);
        double z1 = cos(theta);

        theta = t2 * M_PI;
        phi = p1 * 2.0 * M_PI;
        double x2 = sin(theta) * cos(phi);
        double y2 = sin(theta) * sin(phi);
        double z2 = cos(theta);

        theta = t2 * M_PI;
        phi = p2 * 2.0 * M_PI;
        double x3 = sin(theta) * cos(phi);
        double y3 = sin(theta) * sin(phi);
        double z3 = cos(theta);

        theta = t1 * M_PI;
        phi = p2 * 2.0 * M_PI;
        double x4 = sin(theta) * cos(phi);
        double y4 = sin(theta) * sin(phi);
        double z4 = cos(theta);

        float r = (float)t1;
        float g = (float)p1;
        float b = 1.0f - (float)t1;

        vertices[vidx].point.x = x1; vertices[vidx].point.y = y1; vertices[vidx].point.z = z1;
        vertices[vidx].color.r = r; vertices[vidx].color.g = g; vertices[vidx].color.b = b; vertices[vidx].color.a = 1.0f; vidx++;
        vertices[vidx].point.x = x2; vertices[vidx].point.y = y2; vertices[vidx].point.z = z2;
        vertices[vidx].color.r = r; vertices[vidx].color.g = g; vertices[vidx].color.b = b; vertices[vidx].color.a = 1.0f; vidx++;
        vertices[vidx].point.x = x3; vertices[vidx].point.y = y3; vertices[vidx].point.z = z3;
        vertices[vidx].color.r = r; vertices[vidx].color.g = g; vertices[vidx].color.b = b; vertices[vidx].color.a = 1.0f; vidx++;

        vertices[vidx].point.x = x1; vertices[vidx].point.y = y1; vertices[vidx].point.z = z1;
        vertices[vidx].color.r = r; vertices[vidx].color.g = g; vertices[vidx].color.b = b; vertices[vidx].color.a = 1.0f; vidx++;
        vertices[vidx].point.x = x3; vertices[vidx].point.y = y3; vertices[vidx].point.z = z3;
        vertices[vidx].color.r = r; vertices[vidx].color.g = g; vertices[vidx].color.b = b; vertices[vidx].color.a = 1.0f; vidx++;
        vertices[vidx].point.x = x4; vertices[vidx].point.y = y4; vertices[vidx].point.z = z4;
        vertices[vidx].color.r = r; vertices[vidx].color.g = g; vertices[vidx].color.b = b; vertices[vidx].color.a = 1.0f; vidx++;
      }
    }
  }
  else
  {
    vertex_count = 18;
    vertices = calloc(vertex_count, sizeof(color_point_t));
    if( !vertices )
      return( FALSE );

    int vidx = 0;
    #define SET_VERTEX(idx, px, py, pz, cr, cg, cb, ca) \
      vertices[idx].point.x = px; vertices[idx].point.y = py; vertices[idx].point.z = pz; \
      vertices[idx].color.r = cr; vertices[idx].color.g = cg; vertices[idx].color.b = cb; vertices[idx].color.a = ca

    SET_VERTEX(vidx, 0.0, 0.0, -0.5, 1.0, 0.0, 0.0, 1.0); vidx++;
    SET_VERTEX(vidx, 0.5, 0.0, 0.5, 1.0, 0.0, 0.0, 1.0); vidx++;
    SET_VERTEX(vidx, -0.5, 0.0, 0.5, 1.0, 0.0, 0.0, 1.0); vidx++;

    SET_VERTEX(vidx, 0.0, 0.0, -0.5, 0.0, 1.0, 0.0, 1.0); vidx++;
    SET_VERTEX(vidx, 0.0, 0.5, 0.5, 0.0, 1.0, 0.0, 1.0); vidx++;
    SET_VERTEX(vidx, 0.5, 0.0, 0.5, 0.0, 1.0, 0.0, 1.0); vidx++;

    SET_VERTEX(vidx, 0.0, 0.0, -0.5, 0.0, 0.0, 1.0, 1.0); vidx++;
    SET_VERTEX(vidx, -0.5, 0.0, 0.5, 0.0, 0.0, 1.0, 1.0); vidx++;
    SET_VERTEX(vidx, 0.0, 0.5, 0.5, 0.0, 0.0, 1.0, 1.0); vidx++;

    SET_VERTEX(vidx, 0.0, 0.5, 0.5, 1.0, 1.0, 0.0, 1.0); vidx++;
    SET_VERTEX(vidx, -0.5, 0.0, 0.5, 1.0, 1.0, 0.0, 1.0); vidx++;
    SET_VERTEX(vidx, 0.5, 0.0, 0.5, 1.0, 1.0, 0.0, 1.0); vidx++;

    SET_VERTEX(vidx, -0.5, 0.0, 0.5, 0.5, 0.5, 0.5, 1.0); vidx++;
    SET_VERTEX(vidx, -0.5, -0.5, 0.0, 0.5, 0.5, 0.5, 1.0); vidx++;
    SET_VERTEX(vidx, 0.5, 0.0, 0.5, 0.5, 0.5, 0.5, 1.0); vidx++;

    SET_VERTEX(vidx, 0.5, 0.0, 0.5, 0.3, 0.3, 0.3, 1.0); vidx++;
    SET_VERTEX(vidx, -0.5, -0.5, 0.0, 0.3, 0.3, 0.3, 1.0); vidx++;
    SET_VERTEX(vidx, 0.5, -0.5, 0.0, 0.3, 0.3, 0.3, 1.0); vidx++;

    #undef SET_VERTEX
  }

  out->vertices = vertices;
  out->vertex_count = vertex_count;
  out->vertex_stride = sizeof(color_point_t);
  out->draw_mode = GL_TRIANGLES;
  out->r_max = 1.0f;
  out->zoom = 1.0f;
  out->model_scale = 1.0f;
  out->show_gradient = show_gradient;
  out->generation = generation;

  return( TRUE );
}

static void
triangle_cleanup(void)
{
  if( vertices )
  {
    free(vertices);
    vertices = NULL;
  }
  vertex_count = 0;
}

void
triangle_scene_set_show_gradient(gboolean show)
{
  if( show != show_gradient )
  {
    show_gradient = show;
    generation++;
  }
}

void
triangle_scene_set_mode(int mode)
{
  if( mode != current_mode )
  {
    current_mode = mode;
    generation++;
  }
}

gl_view_config_t triangle_view_config = {
  .vertex_shader_path = "resources/gl/color-vertex.glsl",
  .fragment_shader_path = "resources/gl/color-fragment.glsl",
  .attribs = triangle_attribs,
  .attrib_count = 2,
  .vertex_stride = sizeof(color_point_t),
  .has_gradient = TRUE,
  .gradient_draw = draw_gradient_overlay
};

gl_scene_provider_t triangle_scene_provider = {
  .generate = triangle_generate,
  .post_render = NULL,
  .cleanup = triangle_cleanup
};
