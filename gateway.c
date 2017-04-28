#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include "msgs.h"
#include "linked_list.h"

int sock_fd;
struct sigaction *handler;

void kill_server(int n) {
	close(sock_fd);
	free(handler);
	exit(0);
}

int main(){
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	socklen_t size_addr;
	int nbytes;
	item* server_list = list_init();

	//Action of SIGINT
	handler = malloc(sizeof(handler));
    handler->sa_handler = &kill_server;
	sigaction(SIGINT, handler, NULL);
	/*******************/

	sock_fd= socket(AF_INET, SOCK_DGRAM, 0);

	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}


	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(KNOWN_PORT);
	local_addr.sin_addr.s_addr= INADDR_ANY;

	int err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

	printf(" socket created and binded \n Ready to receive messages\n");

	message_gw *buff = malloc(sizeof(message_gw));

	while(1){
		size_addr = sizeof(client_addr);
		char * stream = malloc(sizeof(message_gw));
		nbytes = recvfrom(sock_fd, stream, sizeof(message_gw), 0,
			(struct sockaddr *) & client_addr, &size_addr);
		memcpy(buff, stream, sizeof(message_gw));
		/*printf("received %d bytes from %s %d --- %s ---\n",
			nbytes, inet_ntoa(client_addr.sin_addr),client_addr.sin_port,  buff);*/
		if(buff->type == 0){
			item* aux = list_first(&server_list);
			if(aux != NULL){
				buff->type = 1;
				strcpy(buff->addr, aux->K.addr);
				buff->port = aux->K.port;
			}
			server_list = list_append(server_list, aux);
			memcpy(stream, buff, sizeof(message_gw));
			nbytes = sendto(sock_fd, stream, sizeof(message_gw), 0,
				(const struct sockaddr *) &client_addr, sizeof(client_addr));
			if( buff->type == 1){
				printf("Client sent to communicate with server %s, port %d\n",
			 		buff->addr, buff->port);
			}
		}else if(buff->type == 1){
			printf("Server addr %s, server port %d, message type %d\n",
				buff->addr, buff->port,  buff->type);
			data K;
			K.port = buff->port;
			strcpy(K.addr, inet_ntoa(client_addr.sin_addr));
			server_list = list_insert(server_list, K);
			printf("Server %s with port %d added to list\n", K.addr, K.port);
		}
    }

	exit(0);
}

int equal_data(data K1, data K2){
		return 0;
}

void print_data(data K){

}
