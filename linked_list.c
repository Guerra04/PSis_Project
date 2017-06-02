#include "linked_list.h"

item* list_init(){
	return NULL;
}

//Removes first element of the list
item* list_first(item** root){
	item *first;

	first = (*root);
	if((*root)!=NULL){
		(*root) = (*root)->next;
		first->next = NULL;
	}else{
		printf("List already empty\n");
	}

	return first;
}

//Puts an element in the beginning of the list
item* list_push(item* root, item* other){

	if(other == NULL)
		return root;

	other->next = root;
	root = other;

	return root;
}

//Creates an element and puts it in the beginning of the list
void list_insert(item** root, data K){
	item *new;

	new = (item*)malloc(sizeof(item));
	if(new == NULL){
		perror("Erro na alocação de new item\n");
		exit(-1);
	}

	new->K = K;
	new->next = *root;
	*root = new;

	return;
}

//Puts element in the end of the list
void list_append(item** root, item* other){
	item *aux = *root;

	if(other == NULL)
		return;

	if(*root == NULL){
		*root = other;
		other->next = NULL;
		return;
	}

	while(aux->next != NULL)
		aux = aux->next;

	aux->next = other;
	other->next = NULL;

	return;
}

//Removes element with data K from the list
int list_remove(item** root, data K){
	item *aux, *aux_seg;

	if((*root) == NULL){
		return 0;
	}

	aux = *root;
	aux_seg = aux->next;
	if(equal_data((*root)->K, K)){
		(*root) = (*root)->next;
		free(aux);
	}else{
		if(aux_seg == NULL){
			return 0;
		}
		while(!equal_data(aux_seg->K, K)){
			if(aux_seg->next == NULL){
				return 0;
			}
			aux = aux->next;
			aux_seg = aux_seg->next;
		}
		aux->next = aux_seg->next;
		free(aux_seg);
	}
	return 1;
}

//Search for item with data K in the list
item* list_search(item** root, data K){
	item *aux;

	if((*root) == NULL){
		return NULL;
	}

	aux = *root;
	while(!equal_data(aux->K, K)){
		if(aux->next == NULL){
			return NULL;
		}
		aux = aux->next;
	}

	if(aux == NULL){
		return NULL;
	}else{
		return aux;
	}

}

//Free all the elements of a list
void list_free(item* root){
	item *aux;

	while(root != NULL){
		aux = root;
		root = root->next;
		free(aux);
	}
	return;
}

//Print all the elements of a list
void list_print(item* root){
	item *aux;
	data k;

	aux = root;
	while(aux != NULL){
		k = aux->K;
		print_data(k);
		aux = aux->next;
	}
	return;
}

//Appends one list to the end of another list
item* lists_concatenate(item* list1, item* list2){
	item* aux=NULL;

	aux = list1;
	if(aux == NULL)
		list1 = list2;
	else{
		while(aux->next != NULL)
			aux = aux->next;
		aux->next = list2;
	}

	return list1;
}

//Counts the number of elements in the list
int list_count(item *root){
	int count=0;
	if(root==NULL)
		return 0;

	item* aux = root;
	count++;
	while(aux->next != NULL){
		count++;
		aux = aux->next;
	}
	return count;
}
