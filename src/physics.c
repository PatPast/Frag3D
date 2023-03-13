#include <stdio.h>
#include <geom.h>
#include <world.h>
#include <config.h>


int resolve_penetration(playerShape_t* player_shape, triangle_t triangle, vector3_t* penetration){
    vector3_t closer_segment_point; // Could be on the segment itself
    float dist_to_plane = get_line_segment_plane_distance(player_shape->segment_up, player_shape->segment_bottom, triangle, &closer_segment_point);
    if (dist_to_plane > player_shape->radius) {
        return 0; // Far from the plane
    }

    float closer_segment_point_distance, upper_tip_distance, lower_tip_distance;
    vector3_t closer_point_projection = project_point_on_triangle_plane(closer_segment_point, triangle, &closer_segment_point_distance);
    vector3_t upper_tip_projection = project_point_on_triangle_plane(player_shape->tip_up, triangle, &upper_tip_distance);
    vector3_t lower_tip_projection = project_point_on_triangle_plane(player_shape->tip_bottom, triangle, &lower_tip_distance);

    if (is_point_in_triangle(closer_point_projection, triangle)
        || is_point_in_triangle(upper_tip_projection, triangle)
        || is_point_in_triangle(lower_tip_projection, triangle)) {
        // The line segment (with its tips) can be projected to the triangle
        // Penetration can be directly calculated via (projection) to (closer_tip)
        vector3_t closer_tip_point;
        if (abs(upper_tip_distance - lower_tip_distance) < 0.00001f) {
            // Equidistance from the both tips indicate a vertical triangle case
            // The tip here is the point on the capsule surface
            closer_tip_point = vector3_sub(player_shape->mid_point, vector3_mult(triangle.normal, player_shape->radius));
        } else if (upper_tip_distance < lower_tip_distance) {
            closer_tip_point = player_shape->tip_up;
        } else if (upper_tip_distance > lower_tip_distance) {
            closer_tip_point = player_shape->tip_bottom;
        }
        
        *penetration = vector3_mult(triangle.normal, vector3_distance(closer_tip_point, closer_point_projection));
        return 1;
    }

    vector3_t closest_point_on_triangle = get_closest_point_on_triangle(closer_segment_point, triangle);
    float dist_to_projection = vector3_length(vector3_sub(closer_point_projection, closest_point_on_triangle));
    if (dist_to_projection > player_shape->radius) {
        return 0;
    }
    // @CLEANUP: This penet_amount is not 100% precise for vertical triangles
    // The exact solution requires getting the (closest_point_on_segment -> closest_point_on_triangle) vector
    // and multiplying with sin of the angle which this vector makes with vector3.down
    // Draw it on something, then it'll be clearer to understand
    float penet_amount = player_shape->radius - dist_to_plane;
    *penetration = vector3_mult(triangle.normal, penet_amount);

    return 1;
}

vector3_t compute_penetrations(vector3_t player_pos, list_t* static_colliders){
    playerShape_t player_shape = playerShape_init(player_pos, player_height, player_radius);
    vector3_t total_displacement;
    list_foreach(sc, static_colliders) {
        staticCollider_t* static_collider = (staticCollider_t*)sc;
        list_foreach(tr, static_collider->triangles) {
            triangle_t triangle = *(triangle_t*)tr;
            vector3_t penet;
            if (resolve_penetration(&player_shape, triangle, &penet)) {
                total_displacement = vector3_add(total_displacement, penet);
                playerShape_displace(&player_shape, penet);
            }
        }
    }

    return total_displacement;
}

int is_grounded(vector3_t player_pos,vector3_t player_move_dir_horz, list_t* static_colliders, vector3_t* ground_normal){
    playerShape_t player_shape = playerShape_init(player_pos, player_height, player_radius);
    vector3_t mid_pos = player_shape.segment_bottom;
    ray_t ground_ray;

    vector3_t left = vector3_cross(VECTOR3_UP, player_move_dir_horz);
    // TODO @PERF: Only three rays at most is enough:
    // One for the move dir, one for mid_pos, one for ghost jump
    list_t* grounded_check_rays = list_init(sizeof(ray_t));

    ground_ray = ray_init(mid_pos, VECTOR3_DOWN),
    list_add(grounded_check_rays, &ground_ray);

    ground_ray = ray_init(vector3_add(mid_pos, vector3_mult(player_move_dir_horz, player_shape.radius)), VECTOR3_DOWN);
    list_add(grounded_check_rays, &ground_ray);

    ground_ray = ray_init(vector3_sub(mid_pos, vector3_mult(player_move_dir_horz, player_shape.radius)), VECTOR3_DOWN);
    list_add(grounded_check_rays, &ground_ray);

    ground_ray = ray_init(vector3_add(mid_pos, vector3_mult(left, player_shape.radius)), VECTOR3_DOWN);
    list_add(grounded_check_rays, &ground_ray);

    ground_ray = ray_init(vector3_sub(mid_pos, vector3_mult(left, player_shape.radius)), VECTOR3_DOWN);
    list_add(grounded_check_rays, &ground_ray);

    float ray_length = player_shape.radius + 0.1f;
    ray_t hit = ray_init(VECTOR3_ZERO, VECTOR3_ZERO);
    list_foreach(r, grounded_check_rays) {
        ray_t ray = *(ray_t*)r;
        if (raycast(ray, ray_length, static_colliders, &hit)) {
            *ground_normal = hit.direction;
            return 1;
        }
    }
    return 0;
}

int raycast(ray_t ray, float max_dist, list_t* static_colliders, ray_t* out){
    list_foreach(c, static_colliders) {
        staticCollider_t* collider = (staticCollider_t*)c;
        list_foreach(tr, collider->triangles) { 
            triangle_t triangle = *(triangle_t*)tr;
            // Ray-triangle check
            float dir_dot_normal = vector3_dot(ray.direction, triangle.normal);
            if (approx(dir_dot_normal, 0)) {
                continue; // Parallel
            }

            float t = vector3_dot(vector3_sub(triangle.p0, ray.origin), triangle.normal) / dir_dot_normal;
            if (t < 0.0f) {
                continue; // Behind the ray
            }

            if (t > max_dist) {
                continue; // Triangle-plane is too far away
            }
            
            vector3_t hit_on_plane = ray_at(ray,t);
            if (is_point_in_triangle(hit_on_plane, triangle)) {
                *out = ray_init(hit_on_plane, triangle.normal);
                return 1;
            }
        }
    }

    return 0;
}