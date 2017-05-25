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


item_r* ring_remove(item_r* root, data_r K){
	item_r *aux, *aux_seg;

	if(root == NULL){
		perror("Already an empty list!\n");
		exit(-1);
	}

	aux = root;
	aux_seg = aux->next;
	if(equal_data_r(root->K, K)){
		root = root->next;
		root->prev = aux->prev;
		aux->prev->next = root;
		free(aux);
	}else{
		while(!equal_data_r(aux_seg->K, K)){
			if(aux_seg->next == root){
				perror("No data_r K found in remove!\n");
				exit(-1);
			}
			aux = aux->next;
			aux_seg = aux_seg->next;
		}
		aux->next = aux_seg->next;
		aux_seg->next->prev = aux;
		free(aux_seg);
	}
	return root;
}
//Removes element with data_r K from the list
/*item_r* ring_remove(item_r* root, data_r K){

    if(root == NULL){
		perror("Already an empty list!\n");
		exit(-1);
	}

    if(root->next == root){
        if(equal_data_r(root->K, K)){
            free(root);
            return NULL;
        }else{
            perror("No data_r K found in remove(1 element)!\n");
            exit(-1);
        }
    }

    item_r *front = root, *back = root->prev;
    int front_found = 0, back_found = 0, not_found = 0;
    #pragma omp parallel sections shared(front, back, front_found, back_found, not_found)
    {
        #pragma omp section
        {
            while(1){
                if(!front_found && !back_found && !not_found){
                    if(equal_data_r(front->K, K)){
                        front_found = 1;
                    }else{
						//just on the front so it won't prevent from seeing all elements
						if(front != back){
                        	front = front->next;
							if(front == root){
								not_found = 1;
								break;
							}
						}else{
							//just on the front so it won't prevent from seeing all elements
							not_found = 1;
							break;
						}
                    }
                }else{
                    break;
                }
            }
        }
        #pragma omp section
        {
            while(1){
                if(!front_found && !back_found && !not_found){
                    if(equal_data_r(back->K, K)){
                        back_found = 1;
                    }else{
                        if(back != front && back->prev != front){
                            back = back->prev;
							if(back == root->prev){
								not_found = 1;
								break;
							}
						}else{
							break;
						}
                    }
                }else{
                    break;
                }
            }
        }
    }
    if(front_found)
        back = front;
    else if(back_found)
        front = back;
    else{
        perror("No data_r K found in remove (multiple elements)!\n");
        exit(-1);
    }

    back = back->prev;
    back->next = front->next;
    front->next->prev = back;
    free(front);
    return root;
}*/

//Search for item_r with data_r K in the list
/*Em principio ta MAL, só nao alterei pq nao usamos
item_r* ring_search(item_r* root, data_r K){

    if(root == NULL){
		printf("Search cancelled, empty list, returning NULL\n");
		return NULL;
	}

    if(root->next == NULL){
        if(equal_data_r(root->K, K))
            return root;
        else{
            printf("Element not found, returning NULL\n");
    		return NULL;
        }
    }

    item_r *front = root, *back = root->prev;
    int front_found = 0, back_found = 0;
	#pragma omp parallel sections shared(front, back, front_found, back_found)
    {
        #pragma omp section
        {
            while(1){
                if(!front_found && !back_found){
                    if(equal_data_r(front->K, K)){
                        front_found = 1;
                    }else{
                        front = front->next;
                    }
                }else{
                    break;
                }
            }
        }
        #pragma omp section
        {
            while(1){
                if(!front_found && !back_found){
                    if(equal_data_r(back->K, K)){
                        back_found = 1;
                    }else{
                        if(back != front && back->prev != front)
                            back = back->prev;
                    }
                }else{
                    break;
                }
            }
        }
    }

    if(front_found){
        return front;
    }else if(back_found){
        return back;
    }else{
        printf("Element not found, returning NULL\n");
        return NULL;
    }
}
*/

//Free all the elements of a list
/*ACHO QUE NAO DÁ PARA PARALELO
void ring_free(item_r* root){

    if(root == NULL){
		printf("Empty list already\n");
		return;
	}

    if(root->next == NULL){
        free(root);
        return;
    }

    item_r *tail = root->prev, *front = root, *back = tail;

    #pragma omp parallel sections shared(front, back, root, tail)
    {
        #pragma omp section
        {
            while(front != back && root != NULL){
                front = root;
                root = root->next;
                free(front);
            }
        }
        #pragma omp section
        {
            while(back != front && tail != NULL){
                back = tail;
                tail = tail->prev;
                free(back);
            }
        }
    }
}
*/

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
