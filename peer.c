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
#include "msgs.h"

//int client_fd;
int sock_fd;
message_tcp * msg;
struct sigaction *handler;

void strupr(char * line);
void *connection(void *client_fd);

void kill_server(int n) {
	close(sock_fd);
	//int dummy = (client_fd!=-1) ? close(client_fd) : client_fd;
	free(msg);
	free(handler);
	exit(0);
}

int main(int argc, char* argv[]){
	struct sockaddr_in server_addr;
	struct sockaddr_in client_gw_addr;
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	int nbytes;
	int err;

	//Action of SIGINT
	handler = malloc(sizeof(handler));
    handler->sa_handler = &kill_server;
	sigaction(SIGINT, handler, NULL);
	/*******************/

/*************Communication with Gateway*****************/
	int sock_fd_gw= socket(AF_INET, SOCK_DGRAM, 0);

	if (sock_fd_gw == -1){
		perror("socket: ");
		exit(-1);
	}

	printf(" socket created \n Ready to send\n");

	int port = 3000 + getpid();

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(KNOWN_PORT_PEER);
	inet_aton(KNOWN_IP, &server_addr.sin_addr);

	message_gw *buff = malloc(sizeof(message_gw));
	strcpy(buff->addr, "127.0.0.1");
	buff->type = 0;
	buff->port = port;
	printf("Server addr %s, server port %d, message type %d\n",
		buff->addr, buff->port,  buff->type);
	char * stream = malloc(sizeof(message_gw));
	memcpy(stream, buff, sizeof(message_gw));
	nbytes = sendto(sock_fd_gw, stream, sizeof(message_gw), 0,
		(const struct sockaddr *) &server_addr, sizeof(server_addr));
	//printf("sent %d %s\n", nbytes, buff);
	/*nbytes = recv(sock_fd_gw, buff, 100, 0);
	printf("received %d bytes --- %s ---\n", nbytes, buff);*/
	free(buff);
	close(sock_fd_gw);

/*****************************SOCKET TCP*****************************/

	sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	socklen_t size_addr;

	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(port);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf(" socket created and binded \n");

	listen(sock_fd, 1);

	printf("Ready to accept connections\n");
	msg = malloc(sizeof(message_tcp));
	while(1){
		int client_fd;
		client_fd = accept(sock_fd, (struct sockaddr *) & client_addr, &size_addr);
		printf("before: %d\n", client_fd);
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
	char *stream = malloc(sizeof(message_tcp));
	int fd = *(int*)client_fd;
	printf("after: %d\n", fd);
	while(recv(fd, stream, sizeof(message_tcp), 0) != EOF){
		printf("Received message from client:\n");
		memcpy(msg, stream, sizeof(message_tcp));
		printf("%s\n", msg->buffer);
		strupr(msg->buffer);
		printf("String converted\n");
		memcpy(stream, msg, sizeof(message_tcp));
		int nbytes = send(fd, stream, sizeof(message_tcp), 0);
		printf("Sent message: %s\n", msg->buffer);
	}
	printf("---------------------------------------------------\n");
	printf("closing connectin with client\n");
	close(fd);
}
