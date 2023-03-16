#ifndef _LIST_H_
#define _LIST_H_

#include <stdlib.h>

/// LISTE

// La structure de données pour représenter un élément de la liste
typedef struct list_elem_s {
    void* data;
    struct list_elem_s* next;
    struct list_elem_s* prev;
} list_elem_t;

// La structure de données pour représenter la liste
typedef struct list_s {
    size_t size;
    size_t data_size;
    list_elem_t* tip;
    list_elem_t* current;
} list_t;

// Initialise une nouvelle liste
list_t* list_init(size_t sizeElem);

// Ajoute un nouvel élément à la fin de la liste
void list_add(list_t* l, void* data);

// Parcourt la liste et applique des instructions à chaque élément
#define list_foreach(_d, _list) \
    void* _d;\
    for(list_elem_t* current = _list->tip->next; (_d = current->data) != NULL && current != _list->tip; current = current->next)


// Retourne une copie de la liste l
list_t* list_duplicate(list_t* l);

// supprime le premier élément de la liste
void list_delete_first(list_t* l);

// supprime le dernier élément de la liste
void list_delete_last(list_t* l);

// Retourne l'élément à l'indice donné 
void* list_elem(list_t* l, int indice);

// Supprime les élements de la liste (attention aux fuites de mémoires)
void list_clear(list_t* l);

// Libère la mémoire utilisée par la liste et detruit cette dernière
void list_destroy(list_t** l);

#endif