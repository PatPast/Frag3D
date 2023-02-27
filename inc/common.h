#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>

/// LISTE

// La structure de données pour représenter un élément de la liste
typedef struct list_elem_s {
    void* data;
    struct list_elem_s* next;
} list_elem_t;

// La structure de données pour représenter la liste
typedef struct list_s {
    size_t size;
    list_elem_t* head;
    list_elem_t* tail;
} list_t;

// Initialise une nouvelle liste
list_t* list_init();

// Ajoute un nouvel élément à la fin de la liste
void list_add(list_t* l, void* data);

// Parcourt la liste et applique une fonction à chaque élément
void list_foreach(list_t* l, void (*func)(void*));

// Libère la mémoire utilisée par la liste
void list_free(list_t* l);

/// FONCTIONS MATHS

inline float random_float(float low, float high) {
    return low + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (high - low)));
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}


#endif