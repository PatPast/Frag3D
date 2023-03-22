#include <stdio.h>
#include <stdlib.h>
#include <list.h>
#include <string.h>

// Initialise une nouvelle liste
list_t* list_init(size_t data_size) {
    list_t* l;
    l = calloc(1, sizeof(list_t));
    l->size = 0;
    l->data_size = data_size;
    l->tip.data = NULL;
    l->tip.next = &l->tip;
    l->tip.prev = &l->tip;
    return l;
}

// Ajoute un nouvel élément à la fin de la liste
void list_add(list_t* l, void* data) {
    if (!data) return; // on n'ajoute d'élément vide dans la liste

    list_elem_t* new_elem = malloc(sizeof(list_elem_t));
    void* new_data;
    if (new_elem == NULL) {
        fprintf(stderr, "Impossible d'allouer de la mémoire\n");
        exit(EXIT_FAILURE);
    }
    new_data = malloc(l->data_size);
    memcpy(new_data, data, l->data_size);

    new_elem->data = new_data;
    new_elem->next = &l->tip; 
    new_elem->prev = l->tip.prev;
    l->tip.prev->next = new_elem;
    l->tip.prev = new_elem;

    l->size++;
}

// Retourne une copie de la liste l
list_t* list_duplicate(list_t* l) {
    list_t* newlist = list_init(l->data_size);
    list_foreach(data, l){
        list_add(newlist, data);
    }
    return newlist;
}

void* list_elem(list_t* l, int indice){
    if(l->size == 0) return NULL;
    list_elem_t* current = l->tip.next;
    for(int i = 0; i < indice; i++){
        current = current->next;
    }
    return current->data;
}

// supprime le premier élément de la liste
void list_delete_first(list_t* l){
    list_elem_t* old = l->tip.next;
    l->tip.next = old->next;
    old->next->prev = &l->tip;
    free(old->data);
    free(old);
}

// supprime le dernier élément de la liste
void list_delete_last(list_t* l){
    list_elem_t* old = l->tip.prev;
    l->tip.prev = old->prev;
    old->prev->next = &l->tip;
    free(old->data);
    free(old);
}


// supprime les élements de la liste (attention aux fuites de mémoires)
void list_clear(list_t* l) {
    while (&l->tip != l->tip.next) list_delete_first(l);
    l->size = 0;
}

// Libère la mémoire utilisée par la liste
void list_destroy(list_t** l) {
    list_clear(*l);
    free(*l);
    *l = NULL;
}

//afficher tout les element de la liste
void list_print(list_t* l, void (*print_elem)(void*)){
    int i = 0;
    list_foreach(elem, l){
        putchar('\t');
        printf("%d -> ", i);
        print_elem(elem);
        putchar('\n');
        i++;
    }
}

