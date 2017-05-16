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
#include "msgs.h"
#include "ring_list.h"

#define DEBUG printf("aqui\n")

int sock_fd_peer;
int sock_fd_client;
struct sigaction *handler;
item* peer_list;

void kill_server(int n) {
	close(sock_fd_peer);
	close(sock_fd_client);
	free(handler);
	exit(0);
}

void *connection_peer(void *args);
void *connection_client(void *args);

int main(){


	socklen_t size_addr;
	int nbytes;
	peer_list = list_init();

	//Action of SIGINT
	handler = malloc(sizeof(handler));
    handler->sa_handler = &kill_server;
	sigaction(SIGINT, handler, NULL);
	/*******************/

	pthread_t thread_peer;
	pthread_t thread_client;

	if(pthread_create(&thread_peer, NULL, connection_peer, NULL) == 0){
		printf("success\n");
	}

	if(pthread_create(&thread_client, NULL, connection_client, NULL) == 0){
		printf("success\n");
	}

	while(1);

	exit(0);
}

void *connection_peer(void *args){
	struct sockaddr_in local_addr;
	struct sockaddr_in peer_addr;
	int nbytes;

	sock_fd_peer = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd_peer == -1){
		perror("socket: ");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(KNOWN_PORT_PEER);
	local_addr.sin_addr.s_addr= INADDR_ANY;

	int err = bind(sock_fd_peer, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf(" socket created and binded \n Ready to receive messages\n");

	message_gw *buff = malloc(sizeof(message_gw));
	char * stream = malloc(sizeof(message_gw));
	while(1){
		socklen_t size_addr = sizeof(peer_addr);
		nbytes = recvfrom(sock_fd_peer, stream, sizeof(message_gw), 0,
				(struct sockaddr *) &peer_addr, &size_addr);
		memcpy(buff, stream, sizeof(message_gw));

		if(buff->type == 0){
			printf("Server addr %s, server port %d, message type %d\n",
					buff->addr, buff->port,  buff->type);
				data K;
				K.port = buff->port;
				strcpy(K.addr, inet_ntoa(peer_addr.sin_addr));
				list_append(&peer_list, K);
				list_print(peer_list);
				printf("Server %s with port %d added to list\n", K.addr, K.port);
		}else{
			//FAZER SERVER
		}
	}
}

void *connection_client(void *args){
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	int nbytes;

	sock_fd_client = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd_client == -1){
		perror("socket: ");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(KNOWN_PORT_CLIENT);
	local_addr.sin_addr.s_addr= INADDR_ANY;

	int err = bind(sock_fd_client, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf(" socket created and binded \n Ready to receive messages\n");

	message_gw *buff = malloc(sizeof(message_gw));

	char * stream = malloc(sizeof(message_gw));
	while(1){
		socklen_t size_addr = sizeof(client_addr);
		nbytes = recvfrom(sock_fd_client, stream, sizeof(message_gw), 0,
				(struct sockaddr *) & client_addr, &size_addr);
		memcpy(buff, stream, sizeof(message_gw));

		item* aux = list_first(&peer_list);
		if(aux != NULL){
			buff->type = 0;
			strcpy(buff->addr, aux->K.addr);
			buff->port = aux->K.port;
			list_append(&peer_list, aux->K);
		}else{
			buff->type = 1;
		}
		printf("%p\n", aux);
		DEBUG;
		memcpy(stream, buff, sizeof(message_gw));
		nbytes = sendto(sock_fd_client, stream, sizeof(message_gw), 0,
			(const struct sockaddr *) &client_addr, sizeof(client_addr));
		if(buff->type == 0){
			printf("Client sent to communicate with server %s, port %d\n",
				buff->addr, buff->port);
		}
	}
}

//Dummy functions
int equal_data(data K1, data K2){
		return 0;
}

void print_data(data K){
	printf("addr = %s, port = %d\n", K.addr, K.port);
	return;
}

item* sort(item* list1, item* list2){
	return NULL;
}
