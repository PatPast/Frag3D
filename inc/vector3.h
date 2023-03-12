#ifndef _VECTOR3_H_
#define _VECTOR3_H_

typedef struct vector3_s {
    float x;
    float y;
    float z;
}vector3_t;

vector3_t vector3_init(float x, float y, float z);

vector3_t vector3_add(vector3_t a, vector3_t b);
vector3_t vector3_sub(vector3_t a, vector3_t b);
vector3_t vector3_mult(vector3_t v, float f);
vector3_t vector3_neg(vector3_t v);
vector3_t vector3_horizontal(vector3_t v);

float vector3_dot(vector3_t a, vector3_t b);
vector3_t vector3_cross(vector3_t a, vector3_t b);
float vector3_length(vector3_t v);
vector3_t vector3_normalize(vector3_t v);
float vector3_distance(vector3_t a, vector3_t b);
vector3_t vector3_rotate_around(vector3_t v, vector3_t axis, float angle);
vector3_t vector3_rotate(vector3_t v, vector3_t euler);

#define VECTOR3_ZERO vector3_init(0, 0, 0)
#define VECTOR3_UP vector3_init(0, 1, 0)
#define VECTOR3_DOWN vector3_init(0, -1, 0)
#define VECTOR3_FORWARD vector3_init(0, 0, 1)
#define VECTOR3_BACK vector3_init(0, 0, -1)
#define VECTOR3_RIGHT vector3_init(1, 0, 0)
#define VECTOR3_LEFT vector3_init(-1, 0, 0)

//void print_vector3(vector3_t v, char* out);

typedef struct matrix4_s {
    float data[16];   
}matrix4_t;

matrix4_t matrix4_init(
    float d0, float d1, float d2, float d3,
    float d4, float d5, float d6, float d7,
    float d8, float d9, float d10, float d11,
    float d12, float d13, float d14, float d15);

matrix4_t matrix4_mult(matrix4_t a, matrix4_t b);
vector3_t matrix4_mult_vector3(matrix4_t m, vector3_t v);

matrix4_t matrix4_rotation(vector3_t euler);
matrix4_t matrix4_look_at(vector3_t eye, vector3_t center, vector3_t up);
matrix4_t matrix4_perspective(float fov, float aspect_ratio, float near, float far);
matrix4_t matrix4_ortho(float left, float right, float bottom, float top, float near, float far);

#define MATRIX4_IDENTITY    matrix4_init(1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1)
#define MATRIX4_ZERO        matrix4_init(0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0)

typedef struct vector2_s {
    float x;
    float y;
}vector2_t;
vector2_t vector2_init(float x, float y);
vector2_t vector2_mult(vector2_t v, float f);
typedef struct vector2i_s {
    int x;
    int y;
}vector2i_t;


#endif