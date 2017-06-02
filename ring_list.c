#include "ring_list.h"
#include <omp.h>

item_r* ring_init(){
	return NULL;
}

//Removes an returns first element of the ring
item_r* ring_first(item_r** root){
	item_r *first;

	first = (*root);
	if((*root)==NULL){
		return first;
    }else if((*root)->next == (*root)){
        (*root) = NULL;
	}else{
        (*root) = (*root)->next;
		(*root)->prev = first->prev;
		first->prev->next = (*root);
	}
	first->next = NULL;
	first->prev = NULL;

	return first;
}

//Puts an element in the end of the ring
void ring_push(item_r** root, item_r* other){

    if(*root == NULL){
        *root = other;
        (*root)->next = *root;
        (*root)->prev = *root;
    }else{
        other->prev = (*root)->prev;
		(*root)->prev->next = other;
    	other->next = *root;
        (*root)->prev = other;
    }
    return;
}

//Creates an element and puts it in the end of the ring
void ring_append(item_r** root, data_r K){
	item_r *new;

	new = (item_r*)malloc(sizeof(item_r));
	if(new == NULL){
		perror("Erro na alocação de new item_r\n");
		exit(-1);
	}

	new->K = K;
	ring_push(root, new);
	return;
}

//Removes element with data equal to K from the ring
//returns 1 if it exists and it was removed, 0 otherwise
int ring_remove(item_r** root, data_r K){
	item_r *aux, *aux_seg;

	if((*root) == NULL){
		return 0;
	}

	aux = (*root);
	aux_seg = aux->next;
	if(equal_data_r((*root)->K, K)){
		if((*root) == aux_seg){
			free((*root));
			(*root) = NULL;
		}else{
			(*root) = (*root)->next;
			(*root)->prev = aux->prev;
			aux->prev->next = (*root);
			free(aux);
		}
	}else{
		if(aux_seg == (*root)){
			printf("No data K found in remove\n");
			return 0;
		}
		while(!equal_data_r(aux_seg->K, K)){
			if(aux_seg->next == (*root)){
				perror("No data_r K found in remove!\n");
				return 0;
			}
			aux = aux->next;
			aux_seg = aux_seg->next;
		}
		aux->next = aux_seg->next;
		aux_seg->next->prev = aux;
		free(aux_seg);
	}
	return 1;
}

//Searches for element with data equal to K on the ring
//returns a reference to that element
item_r* ring_search(item_r* root, data_r K){
	item_r *aux;

	if(root == NULL){
		return NULL;
	}

	aux = root;
	while(!equal_data_r(aux->K, K)){
		if(aux->next == root){
			printf("No data K found in search!\n");
			return NULL;
		}
		aux = aux->next;
	}

	return aux;
}
//Frees all the elements of a ring
void ring_free(item_r* root){
	item_r *aux;
	root = ring_dering(root);
	while(root != NULL){
		aux = root;
		root = root->next;
		free(aux);
	}
	return;
}

//Prints all the elements of a ring
void ring_print(item_r* root){
	item_r *aux;
	data_r k;

	if(root == NULL){
		printf("Empty List!\n");
		return;
	}
	print_data_r(root->K);
	aux = root->next;
	while(aux != root){
		k = aux->K;
		print_data_r(k);
		aux = aux->next;
	}
	return;
}

//Appends one ring to another
//returns a reference to the first element of list1
item_r* rings_concatenate(item_r* list1, item_r* list2){
	item_r *aux;

	if(list1 == NULL && list2 == NULL){
		return NULL;
	}else if(list1 == NULL && list2 != NULL){
		return list2;
	}else if(list1 != NULL && list2 == NULL){
		return list1;
	}else{
	    list2->prev->next = list1;
		list1->prev->next = list2;
		aux = list1->prev;
	    list1->prev = list2->prev;
	    list2->prev = aux;
		return list1;
	}
}

//Turns a ring list into a double linked list (unlink last and first elements)
item_r* ring_dering(item_r *root){

	if(root != NULL){
		root->prev->next = NULL;
		root->prev = NULL;
	}
	return root;
}

//Counts the number of elements in the list
int ring_count(item_r *root){
	int count=0;
	if(root==NULL)
		return 0;

	item_r* aux = root;
	count++;
	while(aux->next != root){
		count++;
		aux = aux->next;
	}
	return count;
}
