#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h> //uint_'s
#include "msgs.h"
#include "linked_list.h"
#include "ring_list.h"

#define ADDR "127.0.0.1"
#define PORT 3000 + getpid()

#define KEYWORD_SIZE 100
#define MAX_PHOTOS 100

#define DEBUG printf("aqui\n")
//int client_fd;
int sock_fd;
int sock_fd_gw;
struct sockaddr_in server_addr;
message_gw *buff;
message_photo * msg;
struct sigaction *handler;
item *photo_list = NULL;
item_r *peer_list = NULL;
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

void strupr(char * line);
void *connection(void *client_fd);
void testing_comm(int fd, message_photo *msg);
int add_photo(int fd, message_photo *msg);
void add_keyword(int fd, message_photo *msg);
void search_photo(int fd, message_photo *msg);
void delete_photo(int fd, message_photo *msg);
void get_photo_name(int fd, message_photo *msg);
void get_photo(int fd, message_photo *msg);
int search_keyword(item *photo, char *keyword);


void kill_server(int n) {
	//Message to send to gateway letting it know that this peer terminated
	if(stream_and_send_gw(sock_fd_gw, &server_addr, ADDR, PORT, -1) == -1)
		exit(1);
	close(sock_fd);
	close(sock_fd_gw);
	list_free(photo_list);
	free(buff);
	free(msg);
	free(handler);
	exit(0);
}

int main(int argc, char* argv[]){
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	int err;

	//Action of SIGINT
	handler = malloc(sizeof(handler));
    handler->sa_handler = &kill_server;
	sigaction(SIGINT, handler, NULL);
	/*******************/

/*************Communication with Gateway*****************/
	sock_fd_gw= socket(AF_INET, SOCK_DGRAM, 0);

	if (sock_fd_gw == -1){
		perror("socket: ");
		exit(-1);
	}

	printf(" socket created \n Ready to send\n");

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(KNOWN_PORT_PEER);
	inet_aton(KNOWN_IP, &server_addr.sin_addr);
	//Sending to gatway this peer's information
	if(stream_and_send_gw(sock_fd_gw, &server_addr, ADDR, PORT, 0) == -1)
		exit(1);

	/*****Waits for acknowledgment of gateway****/
	//sets timeout of recv(...)
	set_recv_timeout(sock_fd_gw, 5, 0);
	//Waits for list of peers
	if( recv_ring_udp(sock_fd_gw, &peer_list) == -1)
		exit(1);

	if(errno == EAGAIN || errno == EWOULDBLOCK){
		//timeout occured
		printf("[ABORTING] The Gateway is not online\n");
		exit(1);
	}
	printf("*********Peers list***********\n");
	ring_print(peer_list);
	printf("******************************\n");

	free(buff);
	//close(sock_fd_gw);

/*****************************SOCKET TCP*****************************/

	sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	socklen_t size_addr;

	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(PORT);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf(" socket created and binded \n");

	listen(sock_fd, 5);

	printf("Ready to accept connections\n");
	msg = malloc(sizeof(message_photo));
	while(1){
		int client_fd;
		client_fd = accept(sock_fd, (struct sockaddr *) & client_addr, &size_addr);
		pthread_t thread_id;

		if(pthread_create(&thread_id, NULL, connection, &client_fd) != 0){
			perror("Creating thread: ");
		}
	}
	close(sock_fd);
	exit(0);
}

void strupr(char * line){
	int i=0;
	while(line[i]!='\0'){
		line[i] = toupper(line[i]);
		i++;
	}
}

void *connection(void *client_fd){
	//printf("Accepted one connection from %s \n", inet_ntoa(client_addr.sin_addr));
	printf("Accepted one connection\n");
	printf("---------------------------------------------------\n");
	int fd = *(int*)client_fd;
	while(recv_and_unstream_photo(fd, msg) != EOF){
		printf("Received message from client:\n");
		switch(msg->type){
			case 0:
				testing_comm(fd, msg);
				break;
			case 1:
				add_photo(fd, msg); //TODO resend if negative
				break;
			case 2:
				add_keyword(fd, msg);
				break;
			case 3:
				search_photo(fd, msg);
				break;
			case 4:
				delete_photo(fd, msg);
				break;
			case 5:
				get_photo_name(fd, msg);
				break;
			case 6:
				get_photo(fd, msg);
				break;
			case -1:
				printf("Client broke connenction!\n");
				close(fd);
				pthread_exit(NULL);
				break;
			default:
				strcpy(msg->buffer,"Type of message undefined!");
				printf("%s\n", msg->buffer);
		}
		//ECHO of the reply
		printf("Sent message: %s\n", msg->buffer);
	}
	printf("---------------------------------------------------\n");
	printf("closing connectin with client\n");
	//TODO nao fechar o peer quando o client fecha a ligação
	close(fd);
	return NULL;
}

void testing_comm(int fd, message_photo *msg){
	printf("%s\n", msg->buffer);
	strupr(msg->buffer);
	printf("String converted\n");
	char *stream = malloc(sizeof(message_photo));
	memcpy(stream, msg, sizeof(message_photo));
	send(fd, stream, sizeof(message_photo), 0);
	free(stream);
}

int add_photo(int fd, message_photo *msg){
	char name[MAX_SIZE];
	char ext[10];
	long size=0;
	uint32_t id=0;
	char photo_name[MAX_SIZE];
	sscanf(msg->buffer,"%[^.].%[^01233456789]%lu", name, ext, &size);

	//Receive photo
	char *bytestream = malloc(size*sizeof(char));
	if(recv_all(fd, bytestream, size, 0) <= 0){
		perror("Receiving: ");
		return -1;
	}

	//Asks Gateway for a new id
	if(stream_and_send_gw(sock_fd_gw, &server_addr, ADDR, PORT, 1) == -1)
		exit(1);

	//Receives id from the gateway
	socklen_t size_addr = sizeof(server_addr);
	if(recvfrom(sock_fd_gw, &id, sizeof(uint32_t), 0, (struct sockaddr *) &server_addr, &size_addr) == -1){
		perror("wtf: ");
		exit(1);
	}

	//Saving photo characteristics in list
	data photo;
	sprintf(photo_name, "%s.%s", name, ext);
	photo = set_data(photo_name, id);
	pthread_mutex_lock(&list_lock);
	list_insert(&photo_list, photo);
	pthread_mutex_unlock(&list_lock);

	list_print(photo_list);
	//Saving photo in disk
	sprintf(photo_name, "%u", id); //CHANGED gravar o nome so com o id, pq depois é mais facil apagar
	FILE *fp;
	if((fp = fopen( photo_name, "wb")) == NULL){
		perror("Opening file to write");
		exit(-1);
	}
	fwrite(bytestream, size, 1, fp);//TODO writes all at once?
	fclose(fp);
	free(bytestream);
	//Sending photo id to client
	if( send_all(fd, &id, sizeof(uint32_t), 0) == -1){
		perror("Sending: ");
		return -2;
	}
	//TODO bcast to other peers
	return id;
}

void add_keyword(int fd, message_photo *msg){
	uint32_t id;
	char keyword[KEYWORD_SIZE];
	int success;

	sscanf(msg->buffer, "%u.%s", &id, keyword);
	data K;
	K.id = id;
	pthread_mutex_lock(&list_lock);
	item* aux = list_search(&photo_list, K);
	pthread_mutex_unlock(&list_lock);

	if(aux == NULL){ //photo with sent id doesn't exist
		success = -2;
	}else{
		if(aux->K.n_keywords < MAX_KEYWORDS){
			//TODO bcast to all peers
			pthread_mutex_lock(&list_lock);
			strcpy(aux->K.keyword[aux->K.n_keywords++],keyword);
			pthread_mutex_unlock(&list_lock);
			success = 1;
		}else{ //keyword list already full
			success = -1;
		}
	}
	if( send_all(fd, &success, sizeof(int), 0) == -1){
		perror("Sending: ");
	}
	return;
}

void search_photo(int fd, message_photo *msg){
	char keyword[KEYWORD_SIZE];
	//TODO verificar scanfs?
	memcpy(keyword, msg->buffer, KEYWORD_SIZE);

	int n_photos = 0;
	item *list = list_init(); //list to store photos
	pthread_mutex_lock(&list_lock);
	item *photo = photo_list;
	while(photo != NULL){
		if(search_keyword(photo, keyword)){
			data K = set_data("", photo->K.id);
			list_insert(&list, K);
			n_photos++;
		}
		photo = photo->next;
	}
	pthread_mutex_unlock(&list_lock);

	//send length of list
	//TODO se houver merda aqui, vai ser foda no cliente
	if( send_all(fd, &n_photos, sizeof(int), 0) == -1){
		perror("Sending: ");
	}

	if(n_photos == 0){
		return;
	}
	//transform list into an array
	uint32_t *id_photos = malloc(n_photos * sizeof(uint32_t));
	for(int i = 0; i < n_photos; i++){
		item *aux = list_first(&list);
		id_photos[i] = aux->K.id;
		free(aux);
	}

	//send array
	if( send_all(fd, id_photos, n_photos * sizeof(uint32_t), 0) == -1){
		perror("Sending: ");
	}
	return;
}

void delete_photo(int fd, message_photo *msg){
	uint32_t id;

	sscanf(msg->buffer, "%u", &id);
	data K;
	K.id = id;
	//TODO mutex
	int found = list_remove(&photo_list, K);
	if(found != 0){ //photo with sent id doesn't exist
		//Removes photo from disk
		char file_name[50];
		sprintf(file_name,"%u", id);
		unlink(file_name);
		//TODO bcast to all peers
	}
	if( send_all(fd, &found, sizeof(int), 0) == -1){
		perror("Sending: ");
	}
	list_print(photo_list);
	return;
}

void get_photo_name(int fd, message_photo *msg){
	uint32_t id;

	sscanf(msg->buffer, "%u", &id);
	data K = set_data("", id); //aux data to search

	pthread_mutex_lock(&list_lock);
	item *aux = list_search(&photo_list, K);
	pthread_mutex_unlock(&list_lock);

	int length = 0;
	if(aux != NULL) //photo exists
		length = strlen(aux->K.name) + 1; //strlen doesn't count with '\0'
	//send length of the photo name
	if( send_all(fd, &length, sizeof(int), 0) == -1){
		perror("Sending: ");
	}

	//send photo name
	if(length > 0){
		if( send_all(fd, aux->K.name, length+1, 0) == -1){
			perror("Sending: ");
		}
	}
	return;
}

void get_photo(int fd, message_photo *msg){

	uint32_t id;

	sscanf(msg->buffer, "%u", &id);
	data K = set_data("", id); //aux data to search

	pthread_mutex_lock(&list_lock);
	item *aux = list_search(&photo_list, K);
	pthread_mutex_unlock(&list_lock);

	FILE *fp;
	long size = 0;
	if(aux != NULL){
		char file_name[MAX_SIZE];
		char ext[10];
		sscanf(aux->K.name, "%*[^.].%s", ext);
		sprintf(file_name, "%u.%s", id, ext);

		if((fp = fopen(file_name, "rb")) == NULL){
			//Invalid filename
			perror("Filename: ");
		}
		fseek(fp, 0, SEEK_END); // jumps to the end of the file
		size = ftell(fp);  // gets the current byte offset in the file
		rewind(fp);
	}
	//Send size of the photo
	if(send_all(fd, &size, sizeof(long), 0) == -1){
		perror("Sending: ");
	}
	char *photo = malloc((size)*sizeof(char));
	fread(photo, size, 1, fp); //reads the whole file at once
	fclose(fp);

	if(send_all(fd, photo, size, 0) == -1){
		perror("Sending: ");
	}
}

/*******************************************************************************
Returns:
	- 1: if keyword exists in photo
	- 0: otherwise
*******************************************************************************/
int search_keyword(item *photo, char *keyword){
	for(int i = 0; i < photo->K.n_keywords; i++){
		if(strcmp(photo->K.keyword[i], keyword) == 0)
			return 1;
	}
	return 0;
}

data set_data(char* name, uint32_t id){
	data K;
	strcpy(K.name, name);
	K.id = id;
	K.n_keywords = 0;
	return K;
}

int equal_data(data K1, data K2){
		return K1.id == K2.id;
}

void print_data(data K){
	printf("photo: name = %s, id = %u, KW = ", K.name, K.id);
	//TODO primeiro inicializar keywords
	/*for(int i = 0; i < 20; i++)
		printf("%s ", K.keyword[i]);*/
	printf("\n");
	return;
}
