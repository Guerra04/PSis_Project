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
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

void strupr(char * line);
void *connection(void *client_fd);
void testing_comm(int fd, message_photo *msg);
int add_photo(int fd, message_photo *msg);
void add_keyword(int fd, message_photo *msg);
void search_photo(int fd, message_photo *meg);
void delete_photo(int fd, message_photo *msg);
int search_keyword(item *photo, char *keyword);


void kill_server(int n) {
	buff = malloc(sizeof(message_gw));
	//Message to send to gateway letting it know that this peer terminated
	strcpy(buff->addr, ADDR);
	buff->type = -1;
	buff->port = PORT;
	char * stream = malloc(sizeof(message_gw));
	memcpy(stream, buff, sizeof(message_gw));
	sendto(sock_fd_gw, stream, sizeof(message_gw), 0,
		(const struct sockaddr *) &server_addr, sizeof(server_addr));

	close(sock_fd);
	close(sock_fd_gw);
	free(buff);
	free(msg);
	free(handler);
	exit(0);
}

int main(int argc, char* argv[]){
	struct sockaddr_in server_addr;
	//struct sockaddr_in client_gw_addr;
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
	buff = malloc(sizeof(message_gw));
	strcpy(buff->addr, ADDR);
	buff->type = 0;
	buff->port = PORT;
	printf("Server addr %s, server port %d, message type %d\n",
		buff->addr, buff->port,  buff->type);
	char * stream = malloc(sizeof(message_gw));
	memcpy(stream, buff, sizeof(message_gw));
	sendto(sock_fd_gw, stream, sizeof(message_gw), 0,
		(const struct sockaddr *) &server_addr, sizeof(server_addr));

	/*****Waits for acknowledgment of gateway****/
	//sets timeout of recv(...)
	struct timeval tv;
	tv.tv_sec = 5;  // 5 seconds timeout
	tv.tv_usec = 0;
	setsockopt(sock_fd_gw, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

	recv(sock_fd_gw, stream, sizeof(message_gw), 0);
	if(errno == EAGAIN || errno == EWOULDBLOCK){
		//timeout occured
		printf("[ABORTING] The Gateway is not online\n");
		exit(1);
	}

	free(buff);
	free(stream);
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
		DEBUG;
		pthread_t thread_id;

		if(pthread_create(&thread_id, NULL, connection, &client_fd) == 0){
			printf("success\n");
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
	char *stream = malloc(sizeof(message_photo));
	int fd = *(int*)client_fd;
	while(recv(fd, stream, sizeof(message_photo), 0) != EOF){
		printf("Received message from client:\n");
		memcpy(msg, stream, sizeof(message_photo));
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
	/*int nbytes = */send(fd, stream, sizeof(message_photo), 0);
	free(stream);
}

int add_photo(int fd, message_photo *msg){
	char name[MAX_SIZE];
	char ext[10];
	long size=0;
	uint32_t id=0;
	char photo_name[MAX_SIZE];
	//TODO recebe nome mal
	sscanf(msg->buffer,"%[^.].%[^01233456789]%lu", name, ext, &size);
	//TODO calc id of photo
	id = 144;
	sprintf(photo_name, "%s.%s", name, ext);
	//Saving photo characteristics in list
	data photo;
	photo = set_data(photo_name, id);
	pthread_mutex_lock(&list_lock);
	list_insert(&photo_list, photo);
	pthread_mutex_unlock(&list_lock);
	//Receive photo
	char *stream = malloc(size*sizeof(char));
	if(recv_all(fd, stream, size, 0) <= 0){
		perror("Receiving: ");
		return -1;
	}
	//Saving photo in disk
	sprintf(photo_name, "%u.%s", id, ext);
	FILE *fp;
	if((fp = fopen( photo_name, "wb")) == NULL){
		perror("Opening file to write");
		exit(-1);
	}
	fwrite(stream, size, 1, fp);//TODO writes all at once?
	fclose(fp);
	free(stream);
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

void search_photo(int fd, message_photo *meg){
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
	int found;

	sscanf(msg->buffer, "%u", &id);
	data K;
	K.id = id;
	item* aux = list_remove(&photo_list, K);

	if(aux == NULL){ //photo with sent id doesn't exist
		found = 0;
	}else{
		found = 1;
		//TODO bcast to all peers
	}
	if( send_all(fd, &found, sizeof(int), 0) == -1){
		perror("Sending: ");
	}
	return;
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
	for(int i = 0; i < 20; i++)
		printf("%s ", K.keyword[i]);
	printf("\n");
	return;
}
