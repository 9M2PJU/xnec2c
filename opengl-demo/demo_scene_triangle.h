#ifndef DEMO_SCENE_TRIANGLE_H
#define DEMO_SCENE_TRIANGLE_H

#include "common.h"
#include "opengl_view.h"

extern gl_view_config_t triangle_view_config;
extern gl_scene_provider_t triangle_scene_provider;

void triangle_scene_set_show_gradient(gboolean show);
void triangle_scene_set_mode(int mode);

#endif
