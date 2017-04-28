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

int sock_fd;
message_tcp * msg;
struct sigaction *handler;

void kill_server(int n) {
	close(sock_fd);
	free(msg);
	free(handler);
	exit(0);
}

int main(int argc, char* argv[]){
	struct sockaddr_in server_gw_addr;
	struct sockaddr_in client_gw_addr;
	struct sockaddr_in server_addr;
	int nbytes;
	int err;

	int sock_fd_gw= socket(AF_INET, SOCK_DGRAM, 0);

	if (sock_fd_gw == -1){
		perror("socket: ");
		exit(-1);
	}

	printf(" socket created \n Ready to send\n");


		/*	client_gw_addr.sin_family = AF_INET;
		client_gw_addr.sin_port = htons(3001);
		client_gw_addr.sin_addr.s_addr = INADDR_ANY;
		err = bind(sock_fd_gw, (struct sockaddr *)&client_gw_addr, sizeof(client_gw_addr));
		if(err == -1) {
		perror("bind");
		exit(-1);
	}

	if(argc < 2){
		printf("Need to include port\n");
		exit(1);
	}
	int port=atoi(argv[1]);
	*/

	server_gw_addr.sin_family = AF_INET;
	server_gw_addr.sin_port = htons(KNOWN_PORT);
	inet_aton(KNOWN_IP, &server_gw_addr.sin_addr);

	message_gw *buff = malloc(sizeof(message_gw));
	buff->type = 0;
	char *stream = malloc(sizeof(message_gw));
	memcpy(stream, buff, sizeof(message_gw));
	nbytes = sendto(sock_fd_gw, stream, sizeof(message_gw), 0,
		(const struct sockaddr *) &server_gw_addr, sizeof(server_gw_addr));

	nbytes = recv(sock_fd_gw, stream, sizeof(message_gw), 0);
	memcpy(buff, stream, sizeof(message_gw));
	printf("Received server info\n");
	close(sock_fd_gw);

/*****************************SOCKET TCP*****************************/

	int sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	socklen_t size_addr;

	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port= htons(buff->port);
	inet_aton(buff->addr, &server_addr.sin_addr);
	free(buff);

	if( -1 == connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr))){
		printf("Error connecting\n");
		exit(-1);
	}
	printf("---------------------------------------------------\n");
	free(stream);
	stream = malloc(sizeof(message_tcp));
	message_tcp *msg = malloc(sizeof(message_tcp));
	memset(msg->buffer, 0, MESSAGE_LEN);
	fgets(msg->buffer,MESSAGE_LEN, stdin);
	memcpy(stream, msg, sizeof(message_tcp));
	nbytes = send(sock_fd, stream, sizeof(message_tcp), 0);
	while(recv(sock_fd, stream, sizeof(message_tcp), 0) != EOF){
		memcpy(msg, stream, sizeof(message_tcp));
		printf("%s\n", msg->buffer);
		memset(msg->buffer, 0, MESSAGE_LEN);
		fgets(msg->buffer,MESSAGE_LEN, stdin);
		memcpy(stream, msg, sizeof(message_tcp));
		nbytes = send(sock_fd, msg, sizeof(message_tcp), 0);
	}
	close(sock_fd);
	exit(0);
}
