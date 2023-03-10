#pragma once

#include <common.h>
#include <platform.h>
#include <assets.h>

struct triangle_s {
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
//TODO finir tout

struct Ray {
    vector3_t origin;
    vector3_t direction;

    explicit Ray(vector3_t o, vector3_t d) : origin(o), direction(d) { }

    vector3_t at(float t){
        return origin + t * direction;
    }
};

struct PlayerShape {
    vector3_t segment_up{};
    vector3_t segment_bottom{};
    vector3_t tip_up{};
    vector3_t tip_bottom{};
    vector3_t mid_point{};
    float radius;

    explicit PlayerShape(vector3_t player_pos, float height, float r) : radius(r) {
        this->mid_point = player_pos;
        this->segment_up = player_pos + (VECTOR3_UP * (height * 0.5f));
        this->segment_bottom = player_pos - (VECTOR3_UP * (height * 0.5f));
        this->tip_up = this->segment_up + VECTOR3_UP * this->radius;
        this->tip_bottom = this->segment_bottom - VECTOR3_UP * this->radius;
    }

    void displace(vector3_t displacement) {
        this->mid_point += displacement;
        this->segment_up += displacement;
        this->segment_bottom += displacement;
        this->tip_up += displacement;
        this->tip_bottom += displacement;
    }
};

struct StaticCollider {
    std::vector<Triangle> triangles;
    explicit StaticCollider(ObjModelData& obj_data,vector3_t position,vector3_t rotation);
};

struct Physics {
    static vector3_t compute_penetrations(vector3_t player_pos,std::vector<StaticCollider>& static_colliders);
    static bool is_grounded(vector3_t player_pos,vector3_t player_move_dir_horz,std::vector<StaticCollider>& static_colliders, vector3_t ground_normal);
    static bool raycast(Ray& ray, float max_dist,std::vector<StaticCollider>& static_colliders, Ray& out);
    static bool resolve_penetration(PlayerShape& player_shape,Triangle& triangle, vector3_t penetration);
};

class World {
    std::vector<StaticCollider> static_colliders;
    void register_static_collider(ObjModelData& obj_data,vector3_t position,vector3_t rotation);

    // Player
    vector3_t player_position{ };
    vector3_t player_forward{ 0, 0, -1 };
    vector3_t player_velocity{ 0, 0, 0 };
    bool is_prev_grounded = false;
    bool fly_move_enabled = false;
    void fly_move(Platform& platform, float dt);
    void mouse_look(Platform& platform, float dt);

public:
    void register_scene(Scene& scene);
    void player_tick(Platform& platform, float dt);
    Matrix4 get_view_matrix() const;
};