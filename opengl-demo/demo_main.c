#include <gtk/gtk.h>
#include "opengl_view.h"
#include "demo_scene_cylinder.h"
#include "demo_scene_triangle.h"
#include "demo_scene_lines.h"

typedef struct
{
  GtkWidget *window;
  GtkWidget *gl_area;
  GtkSpinButton *zoom_spin;
  GtkWidget *share_check;
  arcball_state_t *arcball;
  gboolean is_sharing;
  const char *name;
} window_context_t;

static window_context_t windows[3];
static arcball_state_t *shared_arcball = NULL;

static void
shared_arcball_changed_cb(arcball_state_t *ab, gpointer user_data)
{
  window_context_t *ctx = (window_context_t *)user_data;
  gtk_gl_area_queue_render(GTK_GL_AREA(ctx->gl_area));
}

static void
on_window_share_toggled(GtkToggleButton *button, gpointer user_data)
{
  window_context_t *ctx = (window_context_t *)user_data;
  gboolean share = gtk_toggle_button_get_active(button);
  gl_view_state_t *state;

  state = gl_view_get_state(ctx->gl_area);
  if( !state )
    return;

  if( share && !ctx->is_sharing )
  {
    arcball_remove_callback(ctx->arcball, shared_arcball_changed_cb, ctx);
    state->arcball = shared_arcball;
    arcball_add_callback(shared_arcball, shared_arcball_changed_cb, ctx);
    ctx->is_sharing = TRUE;
  }
  else if( !share && ctx->is_sharing )
  {
    arcball_remove_callback(shared_arcball, shared_arcball_changed_cb, ctx);
    state->arcball = ctx->arcball;
    arcball_add_callback(ctx->arcball, shared_arcball_changed_cb, ctx);
    ctx->is_sharing = FALSE;
  }
  else
  {
    /* Already in desired state */
  }
}

static void
on_cylinder_mode_changed(GtkComboBox *combo, gpointer user_data)
{
  int mode = gtk_combo_box_get_active(combo);
  cylinder_scene_set_mode(mode);
  gtk_gl_area_queue_render(GTK_GL_AREA(windows[0].gl_area));
}

static void
on_triangle_mode_changed(GtkComboBox *combo, gpointer user_data)
{
  int mode = gtk_combo_box_get_active(combo);
  triangle_scene_set_mode(mode);
  gtk_gl_area_queue_render(GTK_GL_AREA(windows[1].gl_area));
}

static void
on_lines_mode_changed(GtkComboBox *combo, gpointer user_data)
{
  int mode = gtk_combo_box_get_active(combo);
  lines_scene_set_mode(mode);
  gtk_gl_area_queue_render(GTK_GL_AREA(windows[2].gl_area));
}

static void
on_gradient_toggled(GtkToggleButton *button, gpointer user_data)
{
  gboolean show = gtk_toggle_button_get_active(button);
  triangle_scene_set_show_gradient(show);
  gtk_gl_area_queue_render(GTK_GL_AREA(windows[1].gl_area));
}

static void
on_zoom_changed(GtkSpinButton *spin, gpointer user_data)
{
  window_context_t *ctx = (window_context_t *)user_data;
  gtk_gl_area_queue_render(GTK_GL_AREA(ctx->gl_area));
}

static GtkWidget*
create_window(int index, const char *title, gl_view_config_t *config,
  gl_scene_provider_t *provider, arcball_state_t *arcball)
{
  GtkWidget *window, *vbox, *hbox, *label;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), title);
  gtk_window_set_default_size(GTK_WINDOW(window), 600, 650);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

  label = gtk_label_new("Zoom:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

  windows[index].zoom_spin = GTK_SPIN_BUTTON(
    gtk_spin_button_new_with_range(0.1, 10.0, 0.1));
  gtk_spin_button_set_value(windows[index].zoom_spin, 1.0);
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(windows[index].zoom_spin), FALSE, FALSE, 5);

  g_signal_connect(windows[index].zoom_spin, "value-changed",
    G_CALLBACK(on_zoom_changed), &windows[index]);

  windows[index].share_check = gtk_check_button_new_with_label("Share");
  gtk_box_pack_start(GTK_BOX(hbox), windows[index].share_check, FALSE, FALSE, 10);
  g_signal_connect(windows[index].share_check, "toggled",
    G_CALLBACK(on_window_share_toggled), &windows[index]);

  windows[index].gl_area = gl_view_create_widget(config, provider, arcball,
    &windows[index].zoom_spin);
  gtk_box_pack_start(GTK_BOX(vbox), windows[index].gl_area, TRUE, TRUE, 0);

  windows[index].window = window;
  windows[index].arcball = arcball;
  windows[index].is_sharing = FALSE;
  windows[index].name = title;

  arcball_add_callback(arcball, shared_arcball_changed_cb, &windows[index]);

  return( window );
}

static void
on_destroy(GtkWidget *widget, gpointer user_data)
{
  gtk_main_quit();
}

int
main(int argc, char *argv[])
{
  GtkWidget *control_window, *vbox, *hbox, *button, *combo, *label;
  arcball_state_t *arcball1, *arcball2, *arcball3;

  gtk_init(&argc, &argv);

  arcball1 = arcball_new();
  arcball_set_view(arcball1, 0.0f, 45.0f);
  arcball2 = arcball_new();
  arcball_set_view(arcball2, 0.0f, 45.0f);
  arcball3 = arcball_new();
  arcball_set_view(arcball3, 0.0f, 45.0f);
  shared_arcball = arcball_new();
  arcball_set_view(shared_arcball, 0.0f, 45.0f);

  create_window(0, "Cylinders (lit-color)", &cylinder_view_config,
    &cylinder_scene_provider, arcball1);
  create_window(1, "Triangles (color + gradient)", &triangle_view_config,
    &triangle_scene_provider, arcball2);
  create_window(2, "Lines (color)", &lines_view_config,
    &lines_scene_provider, arcball3);

  control_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(control_window), "OpenGL Demo Controls");
  gtk_window_set_default_size(GTK_WINDOW(control_window), 400, 300);
  g_signal_connect(control_window, "destroy", G_CALLBACK(on_destroy), NULL);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
  gtk_container_add(GTK_CONTAINER(control_window), vbox);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new("Cylinder Mode:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Radial Spokes");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "RGB Axes");
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
  gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
  g_signal_connect(combo, "changed", G_CALLBACK(on_cylinder_mode_changed), NULL);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new("Triangle Mode:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Sphere");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Pyramid");
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
  gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
  g_signal_connect(combo, "changed", G_CALLBACK(on_triangle_mode_changed), NULL);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  button = gtk_check_button_new_with_label("Show Triangle Gradient Overlay");
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  g_signal_connect(button, "toggled", G_CALLBACK(on_gradient_toggled), NULL);

  gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new("Lines Mode:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Circle");
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Wireframe");
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
  gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);
  g_signal_connect(combo, "changed", G_CALLBACK(on_lines_mode_changed), NULL);

  gtk_box_pack_start(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 5);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  button = gtk_button_new_with_label("Show All Windows");
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  g_signal_connect_swapped(button, "clicked",
    G_CALLBACK(gtk_widget_show_all), windows[0].window);
  g_signal_connect_swapped(button, "clicked",
    G_CALLBACK(gtk_widget_show_all), windows[1].window);
  g_signal_connect_swapped(button, "clicked",
    G_CALLBACK(gtk_widget_show_all), windows[2].window);

  gtk_widget_show_all(control_window);

  gtk_main();

  arcball_free(arcball1);
  arcball_free(arcball2);
  arcball_free(arcball3);
  arcball_free(shared_arcball);

  return( 0 );
}
