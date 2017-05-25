#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h> //close
#include <stdint.h> //uint_'s
#include <netinet/in.h>//in_port_t
#include <string.h> //memcpy
#include <time.h>
#include "msgs.h"

int send_all(int socket, const void *buffer, size_t length, int flags){
	ssize_t nbytes;
	const char *p = buffer;
	while (length > 0){
		nbytes = send(socket, p, length, flags);
		if (nbytes <= 0) break;
		p += nbytes;
		length -= nbytes;
	}
	return (nbytes <= 0) ? -1 : 0; //returns -1 in case of error
}

int recv_all(int socket, void *buffer, size_t max_length, int flags){
	ssize_t nbytes;
	int total_bytes = 0;
	char *p = buffer;
	while (total_bytes < max_length){
		nbytes = recv(socket, p, max_length, flags);
		if (nbytes <= 0) break;
		p += nbytes;
		total_bytes += nbytes;
	}
	if(total_bytes > max_length)
		return -2; //overflow of buffer
	if(nbytes <= 0)
		return nbytes; //error or connection closed
	else
		return total_bytes;

}

int stream_and_send_gw(int fd,struct sockaddr_in* other_addr, char *addr, in_port_t port, int type){
	message_gw buff;
	strcpy(buff.addr, addr);
	buff.type = type;
	buff.port = port;
	char * stream = malloc(sizeof(message_gw));
	memcpy(stream, &buff, sizeof(message_gw));
	if(-1 == sendto(fd, stream, sizeof(message_gw), 0,
		(const struct sockaddr *) other_addr, sizeof(*other_addr))){
			perror("Gateway communication (send): ");
			free(stream);
			return -1;
		}
	free(stream);
	return 0;
}

int stream_and_send_photo(int fd, char *buffer, int type){
	message_photo buff;
	strcpy(buff.buffer, buffer);
	buff.type = type;
	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, &buff, sizeof(message_photo));
	if( send_all(fd, stream, sizeof(message_photo), 0) == -1){
		perror("Photo struct communication (send): ");
		return -1;
	}
	free(stream);
	return 0;
}

void set_recv_timeout(int sock_fd, int secs, int usecs){
	struct timeval tv;
	tv.tv_sec = secs;
	tv.tv_usec = usecs;
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO,
		 (const char*)&tv,sizeof(struct timeval));
}

int recv_and_unstream_gw(int sock_fd,struct sockaddr_in *other_addr, message_gw *buff){
	socklen_t size_addr = sizeof(*other_addr);
	char *stream = malloc(sizeof(message_gw));
	if( -1 == recvfrom(sock_fd, stream, sizeof(message_gw), 0,
			(struct sockaddr *) other_addr, &size_addr)){
				perror("Gateway communication (recv): ");
				free(stream);
				return -1;
			}
	memcpy(buff, stream, sizeof(message_gw));
	free(stream);
	return 0;
}

int recv_and_unstream_photo(int sock_fd, message_photo *buff){
	char *stream = malloc(sizeof(message_photo));
	if( recv_all(sock_fd, stream, sizeof(message_photo), 0) == -1 ){
		//error receiving data
		perror("Photo struct communication (recv): ");
		return -1;
	}
	memcpy(buff, stream, sizeof(message_photo));
	free(stream);
	return 0;
}

int recv_ring_udp(int sock_fd, item_r **peer_list){
	//First receives the size of the list
	int size=0;
	recv(sock_fd, &size, sizeof(int), 0);
	//Then receives list, if it isn't empty
	if(size!=0){
		//Receive vectorized list
		data_r d_recv[size];
		char *stream = malloc(size*sizeof(data_r));
		if(recv(sock_fd, stream, size*sizeof(data_r), 0) == -1){
			perror("List recv: ");
			return -1;
		}
		memcpy(d_recv, stream, size*sizeof(data_r));
		for(int i=0; i < size; i++){
			ring_append(peer_list, d_recv[i]);
		}
	}
	return 0;
}

int send_ring_udp(int sock_fd, struct sockaddr_in* other_addr, item_r *peer_list){
	//Counts elements in peer_list
	int size = ring_count(peer_list);
	sendto(sock_fd, &size, sizeof(int), 0,
		(const struct sockaddr *) other_addr, sizeof(*other_addr));
	//Then sends list, if it isn't empty
	if(size!=0){
		//Vectorizes list
		data_r d_send[size];
		for(int i=0; i < size; i++){
			d_send[i] = peer_list->K;
			peer_list = peer_list->next;
		}
		//Streams and sends peer_list
		char *stream = malloc(size*sizeof(data_r));
		memcpy(stream, d_send, size*sizeof(data_r));
		if(sendto(sock_fd, stream, size*sizeof(data_r), 0,
			(const struct sockaddr *) other_addr, sizeof(*other_addr)) == -1){
			perror("List send: ");
			return -1;
		}
	}
	return 0;
}
