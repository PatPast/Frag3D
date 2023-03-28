#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdlib.h>
#include <vector3.h>

extern size_t point_shadowmap_size;
extern size_t directional_shadowmap_size;
extern float near_plane;
extern float far_plane;
extern float shadow_near_plane;
extern float shadow_far_plane;
extern vector2i_t draw_framebuffer_size;
extern int window_width;
extern int window_height;
extern int max_point_light_count;
extern float player_height;
extern float player_radius;
extern float fov;
extern float fovdefault;

extern float fly_speed;
extern float sensitivity;
extern float max_speed;
extern float gravity;
extern float ground_accel;
extern float ground_friction;
extern float jump_force;
extern float air_accel;
extern float air_decel;
extern float air_control_coeff;
extern float air_control_cpm_coeff;


#endif