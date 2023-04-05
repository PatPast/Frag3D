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


float fly_speed = 30.0f;
float sensitivity = 15.0f;
float max_speed = 12.0f;
float gravity = 16.0f;
float ground_accel = 8.0f;
float ground_friction = 8.0f;
float jump_force = 666.0f;
float air_accel = 1.0f;
float air_decel = 1.0f;
float air_control_coeff = 8.0f;
float air_control_cpm_coeff = 8.0f;