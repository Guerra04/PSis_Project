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
#include "gallery.h"

int sock_fd;
message_photo * msg;
struct sigaction *handler;

void kill_server(int n) {
	close(sock_fd);
	free(msg);
	free(handler);
	exit(0);
}

int main(int argc, char* argv[]){

	/*struct sockaddr_in client_gw_addr;
	int nbytes;
	int err;*/

/*****************Connection to Peer*********************/
	sock_fd =  gallery_connect(KNOWN_IP, KNOWN_PORT_CLIENT);

	switch(sock_fd){
		case -1:
		printf("[ABORTING] Gateway cannot be accessed\n");
		exit(0);
		break;
		case 0:
		printf("[ABORTING] There are no Peers availabe\n");
		exit(0);
		break;
		default:
		printf("Connection to Peer successful\n");
	}
/*****************************Add Photos*****************************/
	printf("---------------------------------------------------\n");

	/*************Communication**************/
	char *stream = malloc(sizeof(message_photo));
	message_photo *msg = malloc(sizeof(message_photo));
	memset(msg->buffer, 0, MAX_SIZE);
	fgets(msg->buffer,MAX_SIZE, stdin);
	memcpy(stream, msg, sizeof(message_photo));
	send(sock_fd, stream, sizeof(message_photo), 0);
	while(recv(sock_fd, stream, sizeof(message_photo), 0) != EOF){
		memcpy(msg, stream, sizeof(message_photo));
		printf("%s\n", msg->buffer);
		memset(msg->buffer, 0, MAX_SIZE);
		fgets(msg->buffer,MAX_SIZE, stdin);
		memcpy(stream, msg, sizeof(message_photo));
		send(sock_fd, msg, sizeof(message_photo), 0);
	}
	/********Communication [END]*****************/
	close(sock_fd);
	exit(0);
}
