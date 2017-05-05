#ifndef LINKED_LIST
#define LINKED_LIST
#include <stdio.h>
#include <stdlib.h>

/***Definir estrutura que contem dados de cada nó**/
struct data{
	char addr[20];
	int port;
};
/**************************************************/
struct item{
	struct data K;
	struct item* next;
};

typedef struct data data;
typedef struct item item;

item* list_init();

item* list_first(item** root);

item* list_push(item* root, item* other);

void list_insert(item** root, data K);

void list_append(item** root, item* other);

item* list_remove(item* root, data K);

item* list_search(item* root, data K);

void list_free(item* root);

void list_print(item* root);

item* lists_concatenate(item* list1, item* list2);

void list_sort(item** root);

void list_split(item* head, item** first_half, item** second_half);

/***** funções abstratas (falta implementação)***/
data set_data(data K);

int equal_data(data K1, data K2); //sucesso=1, insucesso=0

void print_data(data K);

item* sort(item* list1, item* list2);
/*************************************************/

#endif //LINKED_LIST
