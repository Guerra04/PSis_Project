#ifndef _LINKED_LIST_LIB_H_
#define _LINKED_LIST_LIB_H_
#include <stdio.h>
#include <stdlib.h>

/***Definir estrutura que contem dados de cada nó**/
struct data_r{
	char addr[20];
	int port;
};
/**************************************************/
struct item_r{
	struct data_r K;
	struct item_r* next;
    struct item_r* prev;
};

typedef struct data_r data_r;
typedef struct item_r item_r;

item_r* ring_init();

item_r* ring_first(item_r** root);

void ring_push(item_r** root, item_r* other);

void ring_append(item_r** root, data_r K);

item_r* ring_remove(item_r* root, data_r K);

item_r* ring_search(item_r* root, data_r K);

void ring_free(item_r* root);

void ring_print(item_r* root);

item_r* rings_concatenate(item_r* list1, item_r* list2);

void ring_sort(item_r** root);

void ring_split(item_r* head, item_r** first_half, item_r** second_half);

item_r* ring_dering(item_r *root);

/***** funções abstratas (falta implementação)***/
data_r set_data_r(int x, int y, int z);

int equal_data_r(data_r K1, data_r K2); //sucesso=1, insucesso=0

void print_data_r(data_r K);

item_r* sort_r(item_r* list1, item_r* list2);
/*************************************************/

#endif //_LINKED_LIST_LIB_H_
