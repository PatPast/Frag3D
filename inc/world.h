#ifndef _WORLD_H_
#define _WORLD_H_

#include <common.h>
#include <assets.h>
#include <vector3.h>
#include <platform.h>



typedef struct triangle_s {
    vector3_t p0;
    vector3_t p1;
    vector3_t p2;
    vector3_t normal;
    float area;
}triangle_t;

triangle_t triangle_init(vector3_t p0,vector3_t p1,vector3_t p2){
    triangle_t tr = {p0, p1, p2};  
    vector3_t c = vector3_cross(vector3_sub(p1, p0), vector3_sub(p2, p0));
    tr.normal = vector3_normalize(c);
    tr.area = vector3_length(c) * 0.5f;
    return tr;
}


typedef struct ray_s {
    vector3_t origin;
    vector3_t direction;
}ray_t;

ray_t ray_init(vector3_t o, vector3_t d) {
    ray_t r = {o, d};
    return r;
}

vector3_t ray_at(ray_t r, float t){
    return vector3_add(r.origin, vector3_mult(r.direction, t));
}

typedef struct playerShape_s {
    vector3_t segment_up;
    vector3_t segment_bottom;
    vector3_t tip_up;
    vector3_t tip_bottom;
    vector3_t mid_point;
    float radius;
}playerShape_t;

playerShape_t playerShape_init(vector3_t player_pos, float height, float r){
    playerShape_t ps = {.radius = r};
    ps.mid_point = player_pos;
    ps.segment_up = vector3_add(player_pos, vector3_mult(VECTOR3_UP, height * 0.5f));
    ps.segment_bottom = vector3_add(player_pos, vector3_mult(VECTOR3_DOWN, height * 0.5f));
    ps.tip_up = vector3_add(ps.segment_up, vector3_mult(VECTOR3_UP, ps.radius));
    ps.tip_bottom = vector3_add(ps.segment_bottom, vector3_mult(VECTOR3_DOWN, ps.radius));;
    return ps;
}

void playerShape_displace(playerShape_t* ps, vector3_t displacement);

typedef struct staticCollider_s {
    list_t* triangles; //triangle_t
} staticCollider_t;

staticCollider_t* staticCollider_init(objModelData_t* obj_data, vector3_t position, vector3_t rotation);
void staticCollider_freealloc(staticCollider_t* sc);
void staticCollider_destroy(staticCollider_t** sc);
//TODO finir tout

//Physique
vector3_t compute_penetrations(vector3_t player_pos, list_t* static_colliders);
int is_grounded(vector3_t player_pos,vector3_t player_move_dir_horz, list_t* static_colliders, vector3_t* ground_normal);
int raycast(ray_t ray, float max_dist, list_t* static_colliders, ray_t* out);
int resolve_penetration(playerShape_t* player_shape, triangle_t triangle, vector3_t* penetration);


typedef struct world_s {
    list_t* static_colliders; //staticCollider_t

    // Player
    vector3_t player_position;
    vector3_t player_forward;
    vector3_t player_velocity;
    int is_prev_grounded;
    int fly_move_enabled;
}world_t;

world_t* world_init();
void world_freealloc(world_t* w);
void world_destroy(world_t** w);

void world_fly_move(world_t* w, platform_t* platform, float dt);
void world_mouse_look(world_t* w, platform_t* platform, float dt);
matrix4_t world_get_view_matrix(world_t* w);
void world_register_scene(world_t* w, scene_t* scene);
void world_player_tick(world_t* w, platform_t* platform, float dt);
void world_register_static_collider(world_t* w, objModelData_t* obj_data, vector3_t position, vector3_t rotation);

#endif