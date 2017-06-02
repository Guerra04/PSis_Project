//TODO mudar para library.c
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
#include <netinet/in.h>
#include "API.h"
#include "msgs.h"

#define DEBUG printf("aqui\n")

#define DEBUG_PEER(addr,port) printf("peer: addr - %s , port - %d\n", addr,port);

/*******************************************************************************
	Establishes an UDP connection with the gateway and requests the ip
address and port of the TCP socket of one peer. Then it establishes a TCP
connection with said peer and closes the communication with the gateway

Returns:
	- >0: file descriptor of the socket of the TCP connection with the peer
	- 0: no peer is available
	- -1: communication error
*******************************************************************************/
int gallery_connect(char * host, in_port_t port){
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
	if(stream_and_send_gw(sock_fd_gw, &server_gw_addr, "", 0, 0)==-1)
		exit(1);

	//sets timeout of recv
	set_recv_timeout(sock_fd_gw, 5, 0);
	message_gw *buff = malloc(sizeof(message_gw));
	if( recv_and_unstream_gw(sock_fd_gw, &server_gw_addr, buff)==-1 ){
		if(errno == EAGAIN || errno == EWOULDBLOCK){
			//timeout occured
			errno = 0;
			return -1;
		}
		exit(1);
	}
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

	//connect to peer
	if( -1 == connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr))){
		perror("Connecting to Peer: ");
		exit(-1);
	}
	//Sets timeout of socket, so client doesn't stay blocked if peer isnt online
	set_recv_timeout(sock_fd, 8, 0);

	return sock_fd;
}

/*******************************************************************************
	Sends the name and the size of the photo to the peer, receives the id of the
photo and then sends the photo in a byte stream.

Returns:
	- >0: id of the photo that was inserted in the gallery
	- 0: error inserting photo
*******************************************************************************/
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
	char *name_and_size;
	char temp[20];
	sprintf(temp, ",%lu",file_size); // file_size to char
	name_and_size = strcat(file,temp);

	//send name and size of photo
	if(stream_and_send_photo(sock_peer, name_and_size, 1) == -1){
		return 0;
	}
	//Receive photo identifier from Peer
	uint32_t photo_id = 0;
	if( recv_all(sock_peer, &photo_id, sizeof(uint32_t), 0) == -1 ){
		//error receiving data
		perror("Communication: ");
		return 0;
	}

	if(photo_id > 0){
		//Send photo
		char *buffer = (char *)malloc((file_size)*sizeof(char));
		fread(buffer, file_size, 1, fp); //reads the whole file at once
		fclose(fp);
		//send photo
		if( send_all(sock_peer, buffer, file_size, 0) == -1 ){
			//error sending data
			perror("Communication: ");
			return 0;
		}
		return photo_id;
	}else
		return 0;

}

/*******************************************************************************
	Sends id of the photo in which the client wants to insert a keyword and the
keyword to be inserted. Receives an integer to check if the operation was
succesfull or not.

Returns:
	- 1: success
	- 0: communication error
	- -1: keyword list full
	- -2: photo doesn't exist
	- -3: keyword already exists
*******************************************************************************/
int gallery_add_keyword(int peer_socket, uint32_t id_photo, char *keyword){
	char message[MAX_SIZE];
	sprintf(message, "%u.%s", id_photo, keyword);
	//send id and keyword
	if( stream_and_send_photo(peer_socket, message, 2) == -1)
		return 0;

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
	Sends a keyword to the peer. Receives the number of photos that contain the
sent keyword and then receives an array with the ids of those photos and stores
them in 'id_photos'.

Returns:
	- >0: length of photo list with argument keyword
	- 0: no photo with argument keyword
	- -1: communication error
*******************************************************************************/
int gallery_search_photo(int peer_socket, char * keyword, uint32_t ** id_photos){

	char message[MAX_SIZE];
	sprintf(message, "%s", keyword);
	//send keyword
	if( stream_and_send_photo(peer_socket, message, 3) == -1)
		return -1;

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

/*******************************************************************************
	Sends id of the photo to be deleted. Receives an integer to check if the
operation was succesfull or not.

Returns:
	- 1: Photo was deleted
	- 0: Photo with the sent id doesn't exist in the gallery
*******************************************************************************/
int gallery_delete_photo(int peer_socket, uint32_t id_photo){

	char message[MAX_SIZE];
	sprintf(message, "%u", id_photo);
	//send id
	if( stream_and_send_photo(peer_socket, message, 4) == -1)
		return -1;

	//Receive int from peer
	int success;
	if( recv(peer_socket, &success, sizeof(int), 0) == -1){
		//error receiving data
		perror("Communication: ");
		return -1;
	}

	return success;

}

/*******************************************************************************
	Sends id of the photo to be searched. Receives the length of the name of the
photo and then if that length > 0 (photo exists) receives the name of the photo
and stores it in 'photo_name'.

Returns:
	- 1: photo exists and name was retrieved
	- 0: photo doesn't exists
	- -1: communication error
*******************************************************************************/
int gallery_get_photo_name(int peer_socket, uint32_t id_photo, char **photo_name){

	char message[MAX_SIZE];
	sprintf(message, "%u", id_photo);
	//sends id
	if( stream_and_send_photo(peer_socket, message, 5) == -1)
		return -1;

	//Receive the length of the name
	int length = 0;
	if(recv_all(peer_socket, &length, sizeof(int), 0) == -1){
		//error receiving data
		perror("Communication: ");
		return -1;
	}

	if(length == 0){ //no photos with said id
		return 0;
	}else{
		(*photo_name) = malloc(length * sizeof(char));
		//recieve name of the photo
		if(recv_all(peer_socket, (*photo_name), length*sizeof(char), 0) == -1){
			//error receiving data
			perror("Communication: ");
			return -1;
		}
		return 1;
	}
}

/*******************************************************************************
	Sends an id of a photo to the peer. Receives the size of the photo to be
received and then if that size > 0 receives the photo in a byte stream and
writes it to a file with name 'file_name', storing it in the disk.

Returns:
	- 1: photo downloaded succesfully
	- 0: photo does not exist
	- -1: communication error
*******************************************************************************/
int gallery_get_photo(int peer_socket, uint32_t id_photo, char *file_name){

	char message[MAX_SIZE];
	sprintf(message, "%u", id_photo);
	//Send id
	if( stream_and_send_photo(peer_socket, message, 6) == -1)
		return -1;
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
	fwrite(photo, size, 1, fp);
	fclose(fp);
	free(photo);

	return 1;
}
