#include <world.h> 
#include <geom.h>
#include <vector3.h>
#include <config.h>


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
    w->player_velocity = VECTOR3_ZERO;

    //vector3_t d = compute_penetrations(w->player_position, w->static_colliders);
    //w->player_position = vector3_add(w->player_position, d);
}

void world_mouse_look(world_t* w, platform_t* platform, float dt) {
    
    vector2_t mouse_delta = platform_get_mouse_delta(platform);
    float dx = mouse_delta.x;
    float dy = mouse_delta.y;
    //SDL_SetWindowGrab(platform->window, 1); est utile
    


    // TODO @TASK: Up-down angle limits
    // Rename forward to "look_forward" and introduce "body_forward"
    // then check the angle between look forward and body forward

    w->player_forward = vector3_rotate_around(w->player_forward, VECTOR3_UP, -dx * sensitivity * dt);
    vector3_t left = vector3_normalize(vector3_cross(VECTOR3_UP, w->player_forward));
    w->player_forward = vector3_rotate_around(w->player_forward, left, dy * sensitivity * dt);

}

void player_zoom(int active, int zoom, float dt) {
    
    float start, end;

    if (active){
        start = fovdefault;
        end = fovdefault + zoom;
        
    }else{
        end = fovdefault;
        start = fovdefault + zoom;
    }
    fov = end;
    //fov = lerp(start, end + zoom, (end - fov) * dt / end );

    
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

    player_zoom(platform_get_key(platform, k_Zoom), 30, dt);
    vector3_print(w->player_position); putchar('\n');

    if (platform_get_key_down(platform, k_ToggleFly)) {
        w->fly_move_enabled = !w->fly_move_enabled;
    }
    printf("%f\n", dt);
    world_mouse_look(w, platform, dt);
    //printf("%f\n", dt);
    //printf("pos %f %f %f\n", w->player_position.x, w->player_position.y, w->player_position.z); //////////////

    if (w->fly_move_enabled) {
        world_fly_move(w, platform, dt);
        return;
    }

    vector3_t wish_dir = VECTOR3_ZERO;
    vector3_t move_input = VECTOR3_ZERO;
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
        printf("grouded \n");
        int is_gonna_jump = platform_get_key(platform, k_Jump);
        if (w->is_prev_grounded && !is_gonna_jump) {
            apply_friction(&w->player_velocity, dt);
        }

        accelerate(&w->player_velocity, wish_dir, ground_accel, dt);
        w->player_velocity = project_vector_on_plane(w->player_velocity, ground_normal);

        if (is_gonna_jump) {
            printf("jump");
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
/*
void World::player_tick(const Platform& platform, float dt) {
    if (platform.get_key_down(KeyCode::ToggleFly)) {
        this->fly_move_enabled = !this->fly_move_enabled;
    }

    mouse_look(platform, dt);
    //Debug::log("pos %f %f %f", player_position.x, player_position.y, player_position.z);

    if (this->fly_move_enabled) {
        fly_move(platform, dt);
        return;
    }

    Vector3 wish_dir;
    Vector3 move_input;
    Vector3 player_forward_horz = this->player_forward;
    player_forward_horz.y = 0;
    if (platform.get_key(KeyCode::Forward)) {
        wish_dir += player_forward_horz;
        move_input += Vector3::forward;
    } else if (platform.get_key(KeyCode::Back)) {
        wish_dir -= player_forward_horz;
        move_input -= Vector3::forward;
    }

    if (platform.get_key(KeyCode::Left)) {
        wish_dir -= Vector3::cross(player_forward_horz, Vector3::up);
        move_input += Vector3::left;
    } else if (platform.get_key(KeyCode::Right)) {
        wish_dir += Vector3::cross(player_forward_horz, Vector3::up);
        move_input -= Vector3::left;
    }

    if (Vector3::length(wish_dir) > 0.01f) {
        wish_dir = Vector3::normalize(wish_dir);
    }

    const Vector3 horz_vel_dir = Vector3::normalize(this->player_velocity.horizontal());
    Vector3 ground_normal;
    const bool is_grounded = Physics::is_grounded(this->player_position, horz_vel_dir, this->static_colliders, ground_normal);
    if (is_grounded) {
        const bool is_gonna_jump = platform.get_key(KeyCode::Jump);
        if (this->is_prev_grounded && !is_gonna_jump) {
            apply_friction(this->player_velocity, dt);
        }

        accelerate(this->player_velocity, wish_dir, ground_accel, dt);
        this->player_velocity = project_vector_on_plane(this->player_velocity, ground_normal);

        if (is_gonna_jump) {
            this->player_velocity += Vector3::up * jump_force;
        }
    } else {
        const float air_coeff = Vector3::dot(wish_dir, this->player_velocity) > 0.0f ? air_accel : air_decel;
        accelerate(this->wish_dir, player_forward_horz
    const Vector3 displacement = Physics::compute_penetrations(this->player_position, this->static_colliders);
    this->player_position += displacement;
    this->is_prev_grounded = is_grounded;

}
*/