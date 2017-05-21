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
#include "msgs.h"
#include "linked_list.h"

#define ADDR "127.0.0.1"
#define PORT 3000 + getpid()

#define KEYWORD_SIZE 100

#define DEBUG printf("aqui\n")
//int client_fd;
int sock_fd;
int sock_fd_gw;
struct sockaddr_in server_addr;
message_gw *buff;
message_photo * msg;
struct sigaction *handler;
item *photo_list = NULL;

void strupr(char * line);
void *connection(void *client_fd);
void testing_comm(int fd, message_photo *msg);
int add_photo(int fd, message_photo *msg);
void add_keyword(int fd, message_photo *msg);

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
			default:
			strcpy(msg->buffer,"Type of message undefined!");
			printf("%s\n", msg->buffer);
		}
		//ECHO of the reply
		printf("Sent message: %s\n", msg->buffer);
	}
	printf("---------------------------------------------------\n");
	printf("closing connectin with client\n");
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
	long id=0;
	char photo_name[MAX_SIZE];
	//TODO recebe nome mal
	sscanf(msg->buffer,"%[^.].%[^01233456789]%lu", name, ext, &size);
	//TODO calc id of photo
	id = 144;
	sprintf(photo_name, "%s.%s", name, ext);
	//Saving photo characteristics in list
	data photo;
	photo = set_data(photo_name, id);
	list_insert(&photo_list, photo);
	//Receive photo
	char *stream = malloc(size*sizeof(char));
	if(recv_all(fd, stream, size, 0) <= 0){
		printf("Error receiving photo\n");
		return -1;
	}
	//Saving photo in disk
	sprintf(photo_name, "%lu.%s", id, ext);
	FILE *fp;
	if((fp = fopen( photo_name, "wb")) == NULL){
		perror("Opening file to write");
		exit(-1);
	}
	fwrite(stream, size, 1, fp);//TODO writes all at once?
	fclose(fp);
	free(stream);
	//Sending photo id to client
	if( send_all(fd, &id, sizeof(long), 0) == -1){
		printf("Error sending photo\n");
		return -2;
	}
	//TODO bcast to other peers
	return id;
}

void add_keyword(int fd, message_photo *msg){
	long id;
	char keyword[KEYWORD_SIZE];
	int error;

	sscanf(msg->buffer, "%lu.%s", &id, keyword);
	data K;
	K.id = id;
	item* aux = list_search(photo_list, K);

	if(aux == NULL){ //photo with sent id doesn't exist
		error = -2;
	}else{
		if(aux->K.n_keywords < MAX_KEYWORDS){
			//TODO bcast to all peers
			strcpy(aux->K.keyword[aux->K.n_keywords++],keyword);
			error = 0;
		}else{ //keyword list already full
			error = -1;
		}
	}
	send(fd, &error, sizeof(int), 0);
	return;
}

data set_data(char* name, long id){
	data K;
	strcpy(K.name, name);
	K.id = id;
	K.n_keywords = 0;
	return K;
}

int equal_data(data K1, data K2){
		if(K1.id == K2.id)
			return 1;
		else
			return 0;
}

void print_data(data K){
	printf("photo: name = %s, id = %lu, KW = ", K.name, K.id);
	for(int i = 0; i < 20; i++)
		printf("%s ", K.keyword[i]);
	printf("\n");
	return;
}
