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

#define DEBUG_PEER(addr,port) printf("peer: addr - %s , port - %d\n", addr,port);

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
	printf("Received gateway info\n");
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
	DEBUG_PEER(buff->addr, buff->port);
	free(buff);
	free(stream);

	if( -1 == connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr))){
		perror("Connecting to Peer: ");
		printf("%d\n",errno );
		exit(-1);
	}

	return sock_fd;
}


uint32_t gallery_add_photo(int sock_peer, char *file){

	//Get size of photo
	FILE *fp;
	long file_size;

	if((fp = fopen( file, "rb")) == NULL){
		//Invalid filename
		perror("Filename: ");
		return 0;
	}
	fseek(fp, 0, SEEK_END); // jumps to the end of the file
	file_size = ftell(fp);  // gets the current byte offset in the file
	rewind(fp);

	//Send photo size and name
	message_photo *msg = malloc(sizeof(message_photo));
	char *name_and_size = malloc(MAX_SIZE*sizeof(char));
	char temp[20];
	sprintf(temp, "%lu",file_size); // file_size to char
	name_and_size = strcat(file,temp);
	strcpy(msg->buffer, name_and_size);
	msg->type = 1;
	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, msg, sizeof(message_photo));
	if( send_all(sock_peer, stream, sizeof(message_photo), 0) == -1 ){
		//error sending data
		perror("Communication: ");
		return 0;
	}

	//Send photo
	char *buffer = (char *)malloc((file_size)*sizeof(char));
	fread(buffer, file_size, 1, fp); //reads the whole file at once
	fclose(fp);
	if( send_all(sock_peer, buffer, file_size, 0) == -1 ){
		//error sending data
		perror("Communication: ");
		return 0;
	}

	//Receive photo identifier from Peer
	uint32_t photo_id = 0;
	if( recv_all(sock_peer, &photo_id, sizeof(long), 0) == -1 ){
		//error receiving data
		perror("Communication: ");
		return 0;
	}

	//TODO Send photo id (peer side)
	if(photo_id > 0)
		return photo_id;
	else
		return 0;

}


int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword){

	message_photo *msg = malloc(sizeof(message_photo));
	char *id_and_keyword = malloc(MAX_SIZE * sizeof(char));
	sprintf(id_and_keyword, "%u.%s", id_photo, keyword); //concatenate
	strcpy(msg->buffer, id_and_keyword);
	msg->type = 2;

	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, msg, sizeof(message_photo));
	if( send_all(peer_socket, stream, sizeof(message_photo), 0) == -1 ){
		//error sending data
		perror("Communication: ");
		return 0;
	}

	//Receive int from peer
	int error;
	recv(peer_socket, &error, sizeof(int), 0);

	return error;
}
