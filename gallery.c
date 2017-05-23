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

#define DEBUG printf("aqui\n")

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
	if( recv_all(sock_peer, &photo_id, sizeof(uint32_t), 0) == -1 ){
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

/*******************************************************************************
Returns:
	- 1: success
	- 0: communication error
	- -1: keyword list full
	- -2: photo doesn't exist
*******************************************************************************/
int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword){

	message_photo *msg = malloc(sizeof(message_photo));
	sprintf(msg->buffer, "%u.%s", id_photo, keyword);
	//Type of adding a keyword
	msg->type = 2;

	//TODO free stream' em todas as funcs da API
	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, msg, sizeof(message_photo));
	if( send_all(peer_socket, stream, sizeof(message_photo), 0) == -1 ){
		//error sending data
		perror("Communication: ");
		return 0;
	}

	//Receive int from peer
	int success;
	if( recv(peer_socket, &success, sizeof(int), 0) == -1){
		//error receiving data
		perror("Communication: ");
		return 0;
	}

	return success;
}

/*******************************************************************************
Returns:
	- >0: length of photo list with argument keyword
	- 0: no photo with argument keyword
	- -1: communication error
*******************************************************************************/
int gallery_search_photo(int peer_socket, char * keyword, uint32_t ** id_photos){

	message_photo *msg = malloc(sizeof(message_photo));
	sprintf(msg->buffer, "%s", keyword);
	//Type of search
	msg->type = 3;
	//TODO maybe por isto como uma função do msgs.c ja que se usa bue
	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, msg, sizeof(message_photo));
	//send keyword to peer
	if( send_all(peer_socket, stream, sizeof(message_photo), 0) == -1 ){
		//error sending data
		perror("Communication: ");
		return -1;
	}

	//recieve length of the list of photos
	int length = 0;
	if( recv(peer_socket, &length, sizeof(int), 0) == -1){
		//error receiving data
		perror("Communication: ");
		return -1;
	}

	if(length == 0){ //no photos with said keyword
		return 0;
	}

	//receive list of photos
	(*id_photos) = malloc(length * sizeof(uint32_t));
	if( recv(peer_socket, (*id_photos), length*sizeof(uint32_t), 0) == -1){
		//error receiving data
		perror("Communication: ");
		return -1;
	}
	return length;
}


int gallery_delete_photo(int peer_socket, uint32_t id_photo){

	message_photo *msg = malloc(sizeof(message_photo));
	sprintf(msg->buffer, "%u", id_photo);
	//Type of delete
	msg->type = 4;

	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, msg, sizeof(message_photo));
	if( send_all(peer_socket, stream, sizeof(message_photo), 0) == -1 ){
		//error sending data
		perror("Communication: ");
		return -1;
	}

	//Receive int from peer
	int success;
	if( recv(peer_socket, &success, sizeof(int), 0) == -1){
		//error receiving data
		perror("Communication: ");
		return 0;
	}

	return success;

}

/*******************************************************************************
Returns:
	- 1: photo exists and name was retrieved
	- 0: photo doesn't exists
	- -1: communication error
*******************************************************************************/
int gallery_get_photo_name(int peer_socket, uint32_t id_photo, char **photo_name){

	message_photo *msg = malloc(sizeof(message_photo));
	sprintf(msg->buffer, "%u", id_photo);
	//Type of gallery_get_photo_name
	msg->type = 5;

	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, msg, sizeof(message_photo));
	if( send_all(peer_socket, stream, sizeof(message_photo), 0) == -1 ){
		//error sending data
		perror("Communication: ");
		return -1;
	}

	//Receive the length of the name
	int length = 0;
	if( recv(peer_socket, &length, sizeof(int), 0) == -1){
		//error receiving data
		perror("Communication: ");
		return -1;
	}

	if(length == 0){ //no photos with said id
		return 0;
	}else{
		(*photo_name) = malloc(length * sizeof(char));
		if( recv(peer_socket, (*photo_name), length*sizeof(char), 0) == -1){
			//error receiving data
			perror("Communication: ");
			return -1;
		}
		return 1;
	}
}

/*******************************************************************************
Returns:
	- 1: photo downloaded succesfully
	- 0: photo does not exist
	- -1: communication error
*******************************************************************************/
int gallery_get_photo(int peer_socket, uint32_t id_photo, char *file_name){

	message_photo *msg = malloc(sizeof(message_photo));
	sprintf(msg->buffer, "%u", id_photo);
	//Type of gallery_get_photo_name
	msg->type = 6;

	//send photo id and type
	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, msg, sizeof(message_photo));
	if( send_all(peer_socket, stream, sizeof(message_photo), 0) == -1 ){
		//error sending data
		perror("Communication: ");
		return -1;
	}
	//Receive photo size
	long size = 0;
	if( recv(peer_socket, &size, sizeof(long), 0) == -1){
		//error receiving data
		perror("Communication: ");
		return -1;
	}
	if(size == 0) //photo doesn't exist
		return 0;

	//Receive photo
	char *photo = malloc(size * sizeof(char));
	if(recv_all(peer_socket, photo, size, 0) <= 0){
		perror("Receiving: ");
		return -1;
	}

	//Write photo in file to store it in disk
	FILE *fp;
	if((fp = fopen(file_name, "wb")) == NULL){
		perror("Opening file to write");
		exit(-1);
	}
	fwrite(photo, size, 1, fp);//TODO writes all at once?
	fclose(fp);
	free(photo);

	return 1;
}
