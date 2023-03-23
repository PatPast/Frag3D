#include <vector3.h>
#include <math.h>
#include <stdio.h>


#define DEG_TO_RAD 0.0174532925199433

vector3_t vector3_init(float x, float y, float z){
    vector3_t v = {x, y, z};
    return v;
}

vector3_t vector3_add(vector3_t a, vector3_t b){
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}
vector3_t vector3_sub(vector3_t a, vector3_t b){
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}
vector3_t vector3_mult(vector3_t v, float f){
    v.x *= f;
    v.y *= f;
    v.z *= f;
    return v;
}
vector3_t vector3_neg(vector3_t v){
    v.x *= -1;
    v.y *= -1;
    v.z *= -1;
    return v;
}

vector3_t vector3_horizontal(vector3_t v)
{
    v.y = 0;
    return v;
}

float vector3_dot(vector3_t a, vector3_t b){
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
vector3_t vector3_cross(vector3_t a, vector3_t b){
    vector3_t v = { a.y * b.z - a.z * b.y,
        -(a.x * b.z - a.z * b.x),
        a.x * b.y - a.y * b.x };
    return v;
}
float vector3_length(vector3_t v){
    return (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

vector3_t vector3_normalize(vector3_t v){
    return vector3_mult(v, 1 / vector3_length(v));
}
float vector3_distance(vector3_t a, vector3_t b){
    return vector3_length(vector3_sub(a, b));
}
vector3_t vector3_rotate_around(vector3_t v, vector3_t axis, float angle){
    float angle_r = angle * DEG_TO_RAD;
    float cos_angle = cosf(angle_r);
    float sin_angle = sinf(angle_r);
    vector3_t new_v;

    // Rodrigues rotation formula
    new_v = vector3_mult(v, cos_angle);
    new_v = vector3_add(new_v, vector3_mult(vector3_cross(axis, v), sin_angle));
    new_v = vector3_add(new_v, vector3_mult(vector3_mult(axis, vector3_dot(axis, v)), 1 - cos_angle));

    return new_v;
}
vector3_t vector3_rotate(vector3_t v, vector3_t euler){
    v = vector3_rotate_around(v, VECTOR3_LEFT, euler.x);
    v = vector3_rotate_around(v, VECTOR3_UP, euler.y);
    v = vector3_rotate_around(v, VECTOR3_FORWARD, euler.z);
    return v;
}


void vector3_print(vector3_t v){
    printf("{%.2f, %.2f, %.2f}", v.x, v.y, v.z);
}

matrix4_t matrix4_init(
    float d0, float d1, float d2, float d3,
    float d4, float d5, float d6, float d7,
    float d8, float d9, float d10, float d11,
    float d12, float d13, float d14, float d15){
        matrix4_t m = {{d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15}};
        return m;
    }

matrix4_t matrix4_mult(matrix4_t a, matrix4_t b){
    matrix4_t m = MATRIX4_ZERO;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                // Column-major multiplication
                m.data[j * 4 + i] += a.data[k * 4 + i] * b.data[j * 4 + k];
            }
        }
    }

    return m;
}
vector3_t matrix4_mult_vector3(matrix4_t m, vector3_t v){
    vector3_t new_m ={
        m.data[0 * 4 + 0] * v.x + m.data[0 * 4 + 1] * v.y + m.data[0 * 4 + 2] * v.z + m.data[0 * 4 + 3],
        m.data[1 * 4 + 0] * v.x + m.data[1 * 4 + 1] * v.y + m.data[1 * 4 + 2] * v.z + m.data[1 * 4 + 3],
        m.data[2 * 4 + 0] * v.x + m.data[2 * 4 + 1] * v.y + m.data[2 * 4 + 2] * v.z + m.data[2 * 4 + 3]
    };
    return new_m;
}

matrix4_t matrix4_rotation(vector3_t euler){
    matrix4_t m = MATRIX4_ZERO;
    float cos_x = cosf(euler.x * DEG_TO_RAD);
    float cos_y = cosf(euler.y * DEG_TO_RAD);
    float cos_z = cosf(euler.z * DEG_TO_RAD);

    float sin_x = sinf(euler.x * DEG_TO_RAD);
    float sin_y = sinf(euler.y * DEG_TO_RAD);
    float sin_z = sinf(euler.z * DEG_TO_RAD);

    m.data[0 * 4 + 0] = cos_y * cos_z;
    m.data[1 * 4 + 0] = cos_y * sin_z;
    m.data[2 * 4 + 0] = -sin_y;
    m.data[3 * 4 + 0] = 0.0;
    m.data[0 * 4 + 1] = sin_x * sin_y * cos_z - cos_x * sin_z;
    m.data[1 * 4 + 1] = sin_x * sin_y * sin_z + cos_x * cos_z;
    m.data[2 * 4 + 1] = sin_x * cos_y;
    m.data[3 * 4 + 1] = 0.0;
    m.data[0 * 4 + 2] = cos_x * sin_y * cos_z + sin_x * sin_z;
    m.data[1 * 4 + 2] = cos_x * sin_y * sin_z - sin_x * cos_z;
    m.data[2 * 4 + 2] = cos_x * cos_y;
    m.data[3 * 4 + 2] = 0.0;
    m.data[0 * 4 + 3] = 0.0;
    m.data[1 * 4 + 3] = 0.0;
    m.data[2 * 4 + 3] = 0.0;
    m.data[3 * 4 + 3] = 1.0;

    return m;
}


matrix4_t matrix4_look_at(vector3_t eye, vector3_t center, vector3_t up){
    matrix4_t m = MATRIX4_ZERO;

    vector3_t forward = vector3_normalize(vector3_sub(center,eye));
    vector3_t left = vector3_normalize(vector3_cross(forward, up));
    vector3_t local_up = vector3_cross(left, forward);

    m.data[0 * 4 + 0] = left.x;
    m.data[1 * 4 + 0] = left.y;
    m.data[2 * 4 + 0] = left.z;
    m.data[3 * 4 + 0] = -vector3_dot(left, eye);
    m.data[0 * 4 + 1] = local_up.x;
    m.data[1 * 4 + 1] = local_up.y;
    m.data[2 * 4 + 1] = local_up.z;
    m.data[3 * 4 + 1] = -vector3_dot(local_up, eye);
    m.data[0 * 4 + 2] = -forward.x;
    m.data[1 * 4 + 2] = -forward.y;
    m.data[2 * 4 + 2] = -forward.z;
    m.data[3 * 4 + 2] = vector3_dot(forward, eye);
    m.data[0 * 4 + 3] = 0.0;
    m.data[1 * 4 + 3] = 0.0;
    m.data[2 * 4 + 3] = 0.0;
    m.data[3 * 4 + 3] = 1.0;

    return m;
}

matrix4_t matrix4_perspective(float fov, float aspect_ratio, float near, float far){
    matrix4_t m = MATRIX4_ZERO;

    fov *= DEG_TO_RAD;

    float tan_half_fov = tanf(fov * 0.5);

    m.data[0 * 4 + 0] = 1.0 / (aspect_ratio * tan_half_fov);
    m.data[1 * 4 + 1] = 1.0 / tan_half_fov;
    m.data[2 * 4 + 2] = -(far + near) / (far - near);
    m.data[2 * 4 + 3] = -1.0;
    m.data[3 * 4 + 2] = -(2.0 * far * near) / (far - near);
    m.data[3 * 4 + 3] = 0.0;

    return m;
}
matrix4_t matrix4_ortho(float left, float right, float bottom, float top, float near, float far){
    matrix4_t m = MATRIX4_IDENTITY;
    m.data[0 * 4 + 0] = 2 / (right - left);
    m.data[1 * 4 + 1] = 2 / (top - bottom);
    m.data[2 * 4 + 2] = - 2 / (far - near);
    m.data[3 * 4 + 0] = - (right + left) / (right - left);
    m.data[3 * 4 + 1] = - (top + bottom) / (top - bottom);
    m.data[3 * 4 + 2] = - (far + near) / (far - near);

    return m;
}

void matrix4_print(matrix4_t m){
    printf("{%.2f",m.data[0]);
    for (int i = 1; i < 16; i++) printf(", %.2f", m.data[i]);
    printf("}");
}

vector2_t vector2_init(float x, float y){
    vector2_t v = {x, y};
    return v;
}
vector2_t vector2_mult(vector2_t v, float f){
    v.x *= f;
    v.y *= f;
    return v;
}

