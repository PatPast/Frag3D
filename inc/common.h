#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/// LISTE

// La structure de données pour représenter un élément de la liste
typedef struct list_elem_s {
    void* data;
    struct list_elem_s* next;
} list_elem_t;

// La structure de données pour représenter la liste
typedef struct list_s {
    size_t size;
    size_t data_size;
    list_elem_t* head;
    list_elem_t* tail;
} list_t;

// Initialise une nouvelle liste
list_t* list_init(size_t sizeElem);

// Ajoute un nouvel élément à la fin de la liste
void list_add(list_t* l, void* data);

// Parcourt la liste et applique des instructions à chaque élément
#define list_foreach(__elem_data, list) \
    void* __elem_data; \
    for(list_elem_t* elem = list->head; elem != NULL; __elem_data = elem->data, elem = elem->next)


// Libère la mémoire utilisée par la liste
void list_clear(list_t* l);

/// FONCTIONS MATHS



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
typedef struct vector2i_s {
    int x;
    int y;
}vector2i_t;

#endif