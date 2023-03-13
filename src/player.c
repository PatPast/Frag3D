#include <world.h> 
#include <geom.h>
#include <vector3.h>
#include <config.h>


float fly_speed = 30.0f;
float sensitivity = 2.0f;
float max_speed = 12.0f;
float gravity = 16.0f;
float ground_accel = 8.0f;
float ground_friction = 8.0f;
float jump_force = 6.0f;
float air_accel = 1.0f;
float air_decel = 1.0f;
float air_control_coeff = 8.0f;
float air_control_cpm_coeff = 8.0f;

matrix4_t world_get_view_matrix(world_t* w){
    return matrix4_look_at(w->player_position, vector3_add(w->player_position, w->player_forward), VECTOR3_UP);
}

void world_fly_move(world_t* w, platform_t* platform, float dt){
    float displacement = fly_speed * dt;

    if (platform_get_key(platform, k_Forward)) {
        w->player_position = vector3_add(w->player_position, vector3_mult(w->player_forward, displacement));
    }
    if (platform_get_key(platform, k_Back)) {
        w->player_position = vector3_sub(w->player_position, vector3_mult(w->player_forward, displacement));
    }

    if (platform_get_key(platform, k_Left)) {
        w->player_position = vector3_sub(w->player_position, vector3_mult(vector3_normalize(vector3_cross(w->player_forward, VECTOR3_UP)), displacement));
    }
    if (platform_get_key(platform, k_Right)) {
        w->player_position = vector3_add(w->player_position, vector3_mult(vector3_normalize(vector3_cross(w->player_forward, VECTOR3_UP)), displacement));
    }
    if (platform_get_key(platform, k_Up)) {
        w->player_position = vector3_add(w->player_position, vector3_mult(VECTOR3_UP, displacement));
    }
    if (platform_get_key(platform, k_Down)) {
        w->player_position = vector3_sub(w->player_position, vector3_mult(VECTOR3_UP, displacement));
    }
}

void world_mouse_look(world_t* w, platform_t* platform, float dt) {
    vector2_t mouse_delta = platform_get_mouse_delta(platform);
    float dx = mouse_delta.x;
    float dy = mouse_delta.y;

    // TODO @TASK: Up-down angle limits
    // Rename forward to "look_forward" and introduce "body_forward"
    // then check the angle between look forward and body forward

    w->player_forward = vector3_rotate_around(w->player_forward, VECTOR3_UP, -dx * sensitivity * dt);
    vector3_t left = vector3_normalize(vector3_cross(VECTOR3_UP, w->player_forward));
    w->player_forward = vector3_rotate_around(w->player_forward, left, dy * sensitivity * dt);

}

void accelerate(vector3_t* player_velocity, vector3_t wish_dir, float accel_coeff, float dt) {
    float proj_speed = vector3_dot(*player_velocity, wish_dir);
    float add_speed = max_speed - proj_speed;
    if (add_speed < 0.0f) {
        return;
    }

    float accel_amount = accel_coeff * max_speed * dt;
    if (accel_amount > add_speed) {
        accel_amount = add_speed;
    }

    *player_velocity = vector3_add(*player_velocity, vector3_mult(wish_dir, accel_amount));
}

void apply_friction(vector3_t* player_velocity, float dt) {
    float speed = vector3_length(*player_velocity);
    if (speed < 0.001f) {
        *player_velocity = VECTOR3_ZERO;
        return;
    }

    float down_limit = speed > 0.001f ? speed : 0.001f; //max(speed, 0.001f)
    float drop_amount = speed - (down_limit * ground_friction * dt);
    if (drop_amount > 0.0f) {
        *player_velocity = vector3_mult(*player_velocity, drop_amount / speed);
    }
}

void apply_air_control(vector3_t* player_velocity, vector3_t wish_dir, vector3_t move_input, float dt) {
    vector3_t player_dir_horz = vector3_normalize(vector3_horizontal(*player_velocity));
    float player_speed_horz = vector3_length(vector3_horizontal(*player_velocity));

    float dot = vector3_dot(player_dir_horz, wish_dir);
    if (dot > 0) {
        float k = air_control_coeff * dot * dot * dt;

        if (approx(move_input.x, 0)) {
            k *= air_control_cpm_coeff;
        }

        player_dir_horz = vector3_add(vector3_mult(player_dir_horz, player_speed_horz), vector3_mult(wish_dir, k));
        player_dir_horz = vector3_normalize(player_dir_horz);
        *player_velocity = vector3_add(vector3_horizontal(vector3_mult(player_dir_horz, player_speed_horz)), vector3_mult(VECTOR3_UP, player_velocity->y));
    }
}

void world_player_tick(world_t* w, platform_t* platform, float dt) {
    if (platform_get_key_down(platform, k_ToggleFly)) {
        w->fly_move_enabled = !w->fly_move_enabled;
    }

    world_mouse_look(w, platform, dt);
    //Debug::log("pos %f %f %f", player_position.x, player_position.y, player_position.z);

    if (w->fly_move_enabled) {
        world_fly_move(w, platform, dt);
        return;
    }

    vector3_t wish_dir;
    vector3_t move_input;
    vector3_t player_forward_horz = w->player_forward;
    player_forward_horz.y = 0;
    if (platform_get_key(platform, k_Forward)) {
        wish_dir = vector3_add(wish_dir, player_forward_horz);
        move_input = vector3_add(move_input, VECTOR3_FORWARD);
    } else if (platform_get_key(platform, k_Back)) {
        wish_dir = vector3_sub(wish_dir, player_forward_horz);
        move_input = vector3_sub(move_input, VECTOR3_FORWARD);
    }

    if (platform_get_key(platform, k_Left)) {
        wish_dir = vector3_sub(wish_dir, vector3_cross(player_forward_horz, VECTOR3_UP));
        move_input = vector3_add(move_input, VECTOR3_LEFT);
    } else if (platform_get_key(platform, k_Right)) {
        wish_dir = vector3_add(wish_dir, vector3_cross(player_forward_horz, VECTOR3_UP));
        move_input = vector3_sub(move_input, VECTOR3_LEFT);
    }

    if (vector3_length(wish_dir) > 0.01f) {
        wish_dir = vector3_normalize(wish_dir);
    }

    vector3_t horz_vel_dir = vector3_normalize(vector3_horizontal(w->player_velocity));
    vector3_t ground_normal;
    int grounded = is_grounded(w->player_position, horz_vel_dir, w->static_colliders, &ground_normal);
    if (grounded) {
        int is_gonna_jump = platform_get_key(platform, k_Jump);
        if (w->is_prev_grounded && !is_gonna_jump) {
            apply_friction(&w->player_velocity, dt);
        }

        accelerate(&w->player_velocity, wish_dir, ground_accel, dt);
        w->player_velocity = project_vector_on_plane(w->player_velocity, ground_normal);

        if (is_gonna_jump) {
            w->player_velocity = vector3_add(w->player_velocity, vector3_mult(VECTOR3_UP, jump_force));
        }
    } else {
        float air_coeff = vector3_dot(wish_dir, w->player_velocity) > 0.0f ? air_accel : air_decel;
        accelerate(&w->player_velocity, wish_dir, air_coeff, dt);
        if (move_input.z > 0.0001f) {
        }
        apply_air_control(&w->player_velocity, wish_dir, move_input, dt);
        w->player_velocity = vector3_add(w->player_velocity, vector3_mult(VECTOR3_DOWN, gravity * dt));
    }

    w->player_position = vector3_add(w->player_position, vector3_mult(w->player_velocity, dt));

    vector3_t displacement = compute_penetrations(w->player_position, w->static_colliders);
    w->player_position = vector3_add(w->player_position, displacement);
    w->is_prev_grounded = grounded;

}