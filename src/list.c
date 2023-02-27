

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
    list_elem_t* new_node = (list_elem_t*) malloc(sizeof(list_elem_t));
    void* new_data;
    if (new_node == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire\n");
        exit(EXIT_FAILURE);
    }
    new_data = malloc(l->data_size);
    memcpy(new_data, data, l->data_size);
    new_node->data = new_data;
    new_node->next = NULL;

    if (l->tail == NULL) {
        l->head = new_node;
        l->tail = new_node;
    } else {
        l->tail->next = new_node;
        l->tail = new_node;
    }

    l->size++;
}

// Enlève l'élément en tête de la liste et retourne ses données
void* list_pop(list_t* l) {
    if (l->head == NULL) {
        return NULL;
    }

    void* data = l->head->data;
    list_elem_t* old_head = l->head;
    l->head = l->head->next;
    free(old_head);

    l->size--;

    if (l->size == 0) {
        l->tail = NULL;
    }

    return data;
}


// Parcourt la liste et applique une fonction à chaque élément
void list_foreach(list_t* l, void (*func)(void*)) {
    list_elem_t* current_node = l->head;
    while (current_node != NULL) {
        (*func)(current_node->data);
        current_node = current_node->next;
    }
}

// Copie le contenu de la liste b dans la liste a
list_t* list_duplicate(list_t* l) {
    list_t* newlist = list_init(l->data_size);
    void add_to_newlist(void* data){
        void* data_copy;
        data_copy = malloc(l->data_size);
        memcpy(data_copy, data, l->data_size);
        list_add(newlist, data_copy);
    }
    list_foreach(l, add_to_newlist);
    return newlist;
}



// supprime les élements de la liste (attention aux fuites de mémoires)
void list_clear(list_t* l) {
    list_elem_t* current_node = l->head;
    while (current_node != NULL) {
        list_elem_t* next_node = current_node->next;
        free(current_node->data)
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

