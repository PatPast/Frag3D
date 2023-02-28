

#include <stdio.h>
#include <stdlib.h>
#include <common.h>

// Initialise une nouvelle liste
list_t* list_init(size_t data_size) {
    list_t* l;
    l = malloc(sizeof(list_t));
    l->size = 0;
    l->data_size = data_size;
    l->head = NULL;
    l->tail = NULL;
    return l;
}

// Ajoute un nouvel élément à la fin de la liste
void list_add(list_t* l, void* data) {
    list_elem_t* new_elem = (list_elem_t*) malloc(sizeof(list_elem_t));
    void* new_data;
    if (new_elem == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire\n");
        exit(EXIT_FAILURE);
    }
    new_data = malloc(l->data_size);
    memcpy(new_data, data, l->data_size);
    new_elem->data = new_data;
    new_elem->next = NULL;

    if (l->tail == NULL) {
        l->head = new_elem;
        l->tail = new_elem;
    } else {
        l->tail->next = new_elem;
        l->tail = new_elem;
    }

    l->size++;
}

// Copie le contenu de la liste b dans la liste a
list_t* list_duplicate(list_t* l) {
    list_t* newlist = list_init(l->data_size);
    list_foreach(data, l){
        list_add(newlist, data);
    }
    return newlist;
}
//erreur au foreach sans raison

// supprime les élements de la liste (attention aux fuites de mémoires)
void list_clear(list_t* l) {
    list_elem_t* current_node = l->head;
    while (current_node != NULL) {
        list_elem_t* next_node = current_node->next;
        free(current_node->data);
        free(current_node);
        current_node = next_node;
    }
    l->size = 0;
    l->head = NULL;
    l->tail = NULL;
}

// Libère la mémoire utilisée par la liste
void list_destroy(list_t** l) {
    list_clear(*l);
    free(*l);
    *l = NULL;
}

