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

vector3_t vector3_zero;
vector3_t vector3_up;
vector3_t vector3_down;
vector3_t vector3_forward;
vector3_t vector3_back;
vector3_t vector3_left;
vector3_t vector3_right;

//void print_vector3(vector3_t v, char* out);

typedef struct matrix4_s {
    float data[16];   
}matrix4_t;
matrix4_t matrix4_identity;
matrix4_t matrix4_zero;
matrix4_t matrix4_init(float data[16]);
matrix4_t matrix4_mult(matrix4_t a, matrix4_t b);
vector3_t matrix4_mult_vector3(matrix4_t m, vector3_t v);

matrix4_t matrix4_rotation(vector3_t euler);
matrix4_t matrix4_look_at(vector3_t eye, vector3_t center, vector3_t up);
matrix4_t matrix4_perspective(float fov, float aspect_ratio, float near, float far);
matrix4_t matrix4_ortho(float left, float right, float bottom, float top, float near, float far);

/*
typedef struct vector2_s {
    const float x;
    const float y;
};

vector2_t vector2_init(float x, float y,);
vector2_t vector2_mult(vector2_t v, float f);

struct Vector2i {
    const int x;
    const int y;
};
/*

#endif