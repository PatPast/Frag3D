#include <list.h>
#include <stdio.h>

typedef struct test_t{
    int val1;
    float val2;
    char s[256];
}test_t;

void print_test(test_t* test){
    printf("{%d, %.2f, '%s'}\n", test->val1, test->val2, test->s);
}

void print_list(list_t* l){
    if(l->size == 0){
        printf("Liste vide\n");
        return;
    }
    int i = 0;
    list_foreach(e, l){
        test_t* elem = (test_t*)e;
        printf("Element %d :", i++);
        print_test(elem);
    } 
}


int main(){

    test_t var1 = {0, 0.0, ""}; 
    test_t var2 = {10, 4.2, "Hello World!"};
    test_t var3 = {-7, -1.5, "OUAISOUAISOUAIS"}; 
    test_t var4 = {42, 3.14, "POULET KFC"};

    list_t* liste = list_init(sizeof(test_t));

    printf("\nDébut de la phase de test des listes\n");

    printf("----------------------------\n");

    //ajout et affichage des 4 éléments dans la liste
    list_add(liste, &var1);
    list_add(liste, &var2);
    list_add(liste, &var3);
    list_add(liste, &var4);
    print_list(liste);

    printf("----------------------------\n");

    //affichage du premier et dernier élément de la liste
    print_test(liste->tip->next->data);
    print_test(liste->tip->prev->data);


    printf("----------------------------\n");

    list_t* liste_copy = list_duplicate(liste); //copy de 'liste' dans 'liste_copy'
    list_delete_last(liste_copy); //suppression du dernier élément de liste
    print_list(liste_copy);

    printf("----------------------------\n");

    list_clear(liste); //réinitialisation de la liste
    print_list(liste);

    printf("----------------------------\n");

    list_destroy(&liste);
    list_destroy(&liste_copy);

    printf("\nFin de la phase de test des listes\n\n");

    return 0;
}