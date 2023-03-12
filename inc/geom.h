#ifndef _GEOM_H_
#define _GEOM_H_

#include <vector3.h>
#include <stdlib.h>
#include <world.h>

/// GEOMETRY

int in_between(float arg, float f_small, float f_big);
int approx(float arg1, float arg2);
int approx_vec(vector3_t v1, vector3_t v2);

vector3_t project_vector_on_plane(vector3_t v, vector3_t n);
float get_line_segment_plane_distance(vector3_t a, vector3_t b, triangle_t triangle, vector3_t* closer_point);
vector3_t project_point_on_triangle_plane(vector3_t point, triangle_t triangle, float* distance);
int is_point_in_triangle(vector3_t point, triangle_t triangle);
vector3_t get_closest_point_on_triangle(vector3_t point, triangle_t triangle);

void run_geom_tests_internal();

#endif