#include <common.h>
#include <geom.h>
#include <world.h>
#include <vector3.h>
#include <stdio.h>
#include <stdlib.h>


int in_between(float arg, float f_small, float f_big) {
    return arg >= f_small && arg <= f_big;
}

int approx(float arg1, float arg2) {
    return abs(arg1 - arg2) < 0.001f;
}

int approx_vec(vector3_t v1, vector3_t v2) {
    return approx(v1.x, v2.x) && approx(v1.y, v2.y) && approx(v1.z, v2.z);
}


vector3_t project_vector_on_plane(vector3_t v, vector3_t n){
    float sqr_mag = vector3_dot(n, n);
    if (approx(sqr_mag, 0) /*std::isnan(sqr_mag)*/) {
        return v;
    }

    float dot_over_sqr_mag = vector3_dot(v, n) / sqr_mag;
    return vector3_init(v.x - n.x * dot_over_sqr_mag, v.y - n.y * dot_over_sqr_mag, v.z - n.z * dot_over_sqr_mag);
}

float get_line_segment_plane_distance(vector3_t a, vector3_t b, triangle_t triangle, vector3_t* closer_point) {

    float dist_a = abs(vector3_dot(vector3_sub(a, triangle.p0), triangle.normal));
    float dist_b = abs(vector3_dot(vector3_sub(b, triangle.p0), triangle.normal));

    float side_a = vector3_dot(vector3_sub(a, triangle.p0), triangle.normal);
    float side_b = vector3_dot(vector3_sub(b, triangle.p0), triangle.normal);

    if (side_a * side_b < 0) {
        // Directions face towards different sides
        // i.e. line segment is crossing the plane
        float t = dist_a / (dist_a + dist_b);
        *closer_point = vector3_add(a, vector3_mult(vector3_normalize(vector3_sub(b, a)), t));
        return 0;
    }

    if (approx(dist_a, dist_b)) {
        *closer_point = vector3_mult(vector3_add(a, b), 0.5f);
        return dist_a;
    }

    *closer_point = dist_a < dist_b ? a : b;
    return dist_a < dist_b ? dist_a : dist_b; //min(dist_a, dist_b)
}

vector3_t project_point_on_triangle_plane(vector3_t point, triangle_t triangle, float* distance) {
    float dot = vector3_dot(vector3_sub(point, triangle.p0), triangle.normal);
    *distance = abs(dot);
    float side = dot > 0.0f ? -1.0f : 1.0f;
    return vector3_add(point, vector3_mult(triangle.normal, *distance * side));
}

int is_point_in_triangle(vector3_t point, triangle_t triangle) {
    if (abs(vector3_dot(vector3_sub(point, triangle.p0), triangle.normal)) > 0.0001f) {
        //Debug::log("Attempted to perform point-triangle check on non-coplanar point-triangle : \n point %s\n\
        //    triangle p0: %s \ntriangle normal: %s", point.to_string(), triangle.p0.to_string(), triangle.normal.to_string());
        return 0;
    }

    // Barycentric coordinates
    float a = vector3_length(vector3_cross(vector3_sub(triangle.p1, point), vector3_sub(triangle.p2, point))) / (2.0f * triangle.area);
    float b = vector3_length(vector3_cross(vector3_sub(triangle.p2, point), vector3_sub(triangle.p0, point))) / (2.0f * triangle.area);
    float c = vector3_length(vector3_cross(vector3_sub(triangle.p0, point), vector3_sub(triangle.p1, point))) / (2.0f * triangle.area);

    return in_between(a, 0.0f, 1.0f) && in_between(b, 0.0f, 1.0f) && in_between(c, 0.0f, 1.0f)
        && approx(a + b + c, 1.0f);
}

vector3_t get_closest_point_on_line_segment(vector3_t point, vector3_t a, vector3_t b, float* distance) {
    vector3_t line_segment_dir = vector3_normalize(vector3_sub(b, a));
    float line_segment_length = vector3_length(vector3_sub(b, a));

    float dot = vector3_dot(vector3_sub(point, a), line_segment_dir);
    if (in_between(dot, 0, line_segment_length)) {
        *distance = dot;
        return vector3_add(a, vector3_mult(line_segment_dir, dot));
    }

    float dist1 = vector3_distance(point, a);
    float dist2 = vector3_distance(point, b);

    if (dist1 < dist2) {
        *distance = dist1;
        return a;
    } 

    *distance = dist2;
    return b;
}

vector3_t get_closest_point_on_triangle(vector3_t point, triangle_t triangle) {
    float d0, d1, d2;
    vector3_t p0 = get_closest_point_on_line_segment(point, triangle.p0, triangle.p1, &d0);
    vector3_t p1 = get_closest_point_on_line_segment(point, triangle.p1, triangle.p2, &d1);
    vector3_t p2 = get_closest_point_on_line_segment(point, triangle.p2, triangle.p0, &d2);

    if (d0 < d1 && d0 < d2) {
        return p0;
    }
    if (d1 < d2) {
        return p1;
    }
    return p2;
}