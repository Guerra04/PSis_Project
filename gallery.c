#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h> //close
#include <stdint.h> //uint_'s
#include <string.h> //memcpy
#include "gallery.h"
#include "msgs.h"

int gallery_connect(char * host, uint32_t port){
	/****************Gateway communication***************/
	//Creation of UDP socket
	int sock_fd_gw= socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd_gw == -1){
		perror("socket: ");
		exit(-1);
	}
	//Gateway address and port atribution
	struct sockaddr_in server_gw_addr;
	server_gw_addr.sin_family = AF_INET;
	server_gw_addr.sin_port = htons(port);
	inet_aton(host, &server_gw_addr.sin_addr);
	//Send request of Peer to Gateway
	message_gw *buff = malloc(sizeof(message_gw));
	buff->type = 0;
	char *stream = malloc(sizeof(message_gw));
	memcpy(stream, buff, sizeof(message_gw));
	sendto(sock_fd_gw, stream, sizeof(message_gw), 0,
		(const struct sockaddr *) &server_gw_addr, sizeof(server_gw_addr));

	//sets timeout of recv(...)
	struct timeval tv;
	tv.tv_sec = 5;  // 5 seconds timeout
	tv.tv_usec = 0;
	setsockopt(sock_fd_gw, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

	recv(sock_fd_gw, stream, sizeof(message_gw), 0);
	if(errno == EAGAIN || errno == EWOULDBLOCK){
		//timeout occured
		return -1;
	}
	memcpy(buff, stream, sizeof(message_gw));
	printf("Received server info\n");
	close(sock_fd_gw);
	if( buff->type == 1){
		//no Peer is available
		return 0;
	}

	/********************Socket TCP Creation***************************/
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port= htons(buff->port);
	inet_aton(buff->addr, &server_addr.sin_addr);
	free(buff);
	free(stream);

	if( -1 == connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr))){
		perror("Connecting to Peer: ");
		exit(-1);
	}

	return sock_fd;
}
