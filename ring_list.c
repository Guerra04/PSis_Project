#include "ring_list.h"
#include <omp.h>

item_r* ring_init(){
	return NULL;
}

//Removes first element of the list
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

//Puts an element in the beginning of the list
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
    	*root = other;
    }
    return;
}

//Creates an element and puts it in the beginning of the list
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


int ring_remove(item_r** root, data_r K){
	item_r *aux, *aux_seg;

	if((*root) == NULL){
		perror("Already an empty list!\n");
		exit(-1);
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

//Free all the elements of a list
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

//Print all the elements of a list
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

//Appends one list to the end of another list
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
//Recursively divide the list in half and sort the sublists
//Needs to be called in a parallel section
void ring_sort(item_r** root){
	item_r* first_half;
	item_r* second_half;
	item_r* tmphead = *root;

	/*Empty list or with only one element*/
	if((tmphead == NULL) || (tmphead->next == NULL)){
		return;
	}

	/*create the 2 sublists*/
	ring_split(tmphead, &first_half, &second_half);

	/*recursively sort the 2 sublists*/
	ring_sort(&first_half);
	ring_sort(&second_half);

	/*sort and merge the sublists together*/
	*root = sort_r(first_half, second_half);
	//return;
}

/*Divides the list in half
Uses 1 pointer that advances 2 elements (fast) and
1 pointer that only advances 1(slow)*/
void ring_split(item_r* head, item_r** first_half, item_r** second_half){
	item_r* slow;
	item_r* fast;

	/*Empty list or with only one element*/
	if((head == NULL) || (head->next == NULL)){
		*first_half = head;
		*second_half = NULL;
	}else{
		slow = head;
		fast = head->next;

		while(fast != NULL){
			fast = fast->next;
			if(fast != NULL){
				slow = slow->next;
				fast = fast->next;
			}
		}

		/*slow is in before the element in the middle*/
		*first_half = head;
		*second_half = slow->next;
		slow->next = NULL;
	}
}

item_r* ring_dering(item_r *root){

	if(root != NULL){
		root->prev->next = NULL;
		root->prev = NULL;
	}
	return root;
}

int ring_count(item_r *root){
	int count=0;
	if(root==NULL)
		return 0;

	item_r* aux = root;
	count++;
	while(aux->seg != root){
		count++;
		aux = aux->next;
	}
	return count;
}
