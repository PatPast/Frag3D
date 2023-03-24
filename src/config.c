#include <config.h>

size_t point_shadowmap_size = 1024;
size_t directional_shadowmap_size = 2048;
float near_plane = 0.001f;
float far_plane = 1000.0f;
float shadow_near_plane = 0.001f;
float shadow_far_plane = 1000.0f;
vector2i_t draw_framebuffer_size = {640, 360};
int window_width = 1080;
int window_height = 720;
int max_point_light_count = 10;
float player_height = 2.0f;
float player_radius = 0.5f;
float fov = 130.0f;
float fovdefault = 130.0f;
