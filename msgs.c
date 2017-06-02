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

#define DEBUG printf("aqui\n")


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

int stream_and_send_photo(int fd, const char *buffer, int type){
	message_photo buff;
	strcpy(buff.buffer, buffer);
	buff.type = type;
	char * stream = malloc(sizeof(message_photo));
	memcpy(stream, &buff, sizeof(message_photo));
	if( send_all(fd, stream, sizeof(message_photo), 0) == -1){
		perror("Photo struct communication (send): ");
		return -1;
	}//TODO print seand_all
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

void reset_recv_timeout(int sock_fd){
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO,
		 (const char*)&tv,sizeof(struct timeval));
}

int recv_and_unstream_gw(int sock_fd,struct sockaddr_in *other_addr, message_gw *buff){
	socklen_t size_addr = sizeof(*other_addr);
	char *stream = malloc(sizeof(message_gw));
	int read = recvfrom(sock_fd, stream, sizeof(message_gw), 0,
			(struct sockaddr *) other_addr, &size_addr);
	if(read == -1){
		perror("Gateway communication (recv): ");
		free(stream);
		return -1;
	}else if(read == 0){
		free(stream);
		return 0;
	}
	memcpy(buff, stream, sizeof(message_gw));
	free(stream);
	return 1;
}

int recv_and_unstream_photo(int sock_fd, message_photo *buff){
	char *stream = malloc(sizeof(message_photo));
	//TODO mudar tudo isto para diferenciar close connection de erro
	int read = recv_all(sock_fd, stream, sizeof(message_photo), 0);
	if(read == -1){
		//error receiving data
		free(stream);
		perror("Photo struct communication (recv): ");
		return -1;
	}else if(read == 0){
		free(stream);
		return -0;
	}
	memcpy(buff, stream, sizeof(message_photo));
	free(stream);
	return 1;
}

int recv_ring_udp(int sock_fd, item_r **peer_list){
	//First receives the size of the list
	int size=0;
	if(recv(sock_fd, &size, sizeof(int), 0) == -1){
		perror("Size recv: ");
		return -1;
	}
	//Then receives list, if it isn't empty
	if(size!=0){
		//Receive vectorized list
		data_r d_recv[size];
		char *stream = malloc(size*sizeof(data_r));
		if(recv(sock_fd, stream, size*sizeof(data_r), 0) <= 0){
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

int recv_list_tcp(int sock_fd, item** list){
	//First receives the size of the list
	int size=0;
	if(recv_all(sock_fd, &size, sizeof(int), 0) == -1){
		perror("Receive size: ");
		return -1;
	}
	//Then receives list, if it isn't empty
	if(size!=0){
		//Receive vectorized list
		data d_recv[size];
		char *stream = malloc(size*sizeof(data));
		if(recv(sock_fd, stream, size*sizeof(data), 0) == -1){
			perror("Receive list: ");
			return -1;
		}
		memcpy(d_recv, stream, size*sizeof(data));
		for(int i=0; i < size; i++){
			list_insert(list, d_recv[i]);
		}
	}
	return 0;
}

int send_list_tcp(int sock_fd, item *photo_list){

	int size = list_count(photo_list);
	if(send_all(sock_fd, &size, sizeof(int), 0) == -1){
		perror("Send size: ");
		return -1;
	}

	if(size!=0){
		//Vectorizes list
		data d_send[size];
		for(int i=0; i < size; i++){
			d_send[i] = photo_list->K;
			photo_list = photo_list->next;
		}
		//Streams and sends peer_list
		char *stream = malloc(size*sizeof(data));
		memcpy(stream, d_send, size*sizeof(data));
		if(send_all(sock_fd, d_send, size*sizeof(data), 0) == -1){
			perror("Send list: ");
			return -1;
		}
	}
	return 0;
}

/******************************************************************************
 * Checks if someone is listening on the address host:port.
 * Returns: TCP socket created if someone's listening.
 * 			0 - if noone's listening
 *****************************************************************************/
int isOnline(char * host, in_port_t port){
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port= htons(port);
	inet_aton(host, &server_addr.sin_addr);
	//connect sets errno to ECONNREFUSED if no one is listening on the remote address
	if( connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr))==-1 ){
		if(errno == ECONNREFUSED){
			printf("[PeerLoss] Peer %s with port %d is not online\n", host, port);
			close(sock_fd);
			return 0;
		}
		perror("Connecting peer");
		exit(1);
	}

	return sock_fd;
}

//Dummy functions for linked list
data set_data(char* name, uint32_t id){
	data K;
	strcpy(K.name, name);
	K.id = id;
	K.n_keywords = 0;
	for(int i=0 ; i < MAX_KEYWORDS; i++){
		K.keyword[i][0] = '\0';
	}
	return K;
}

int equal_data(data K1, data K2){
		return K1.id == K2.id;
}

void print_data(data K){
	printf("photo: name = %s, id = %u, KW =", K.name, K.id);
	for(int i = 0; i < K.n_keywords; i++){
			printf(" %s ,", K.keyword[i]);
	}
	printf("\n");
	return;
}
//Dummy functions for ring list
data_r set_data_r(char *addr, int port){
	data_r K;
	K.port = port;
	strcpy(K.addr, addr);
	return K;
}

int equal_data_r(data_r K1, data_r K2){
		if(strcmp(K1.addr, K2.addr) == 0 && K1.port == K2.port)
			return 1;
		else
			return 0;
}

void print_data_r(data_r K){
	printf("addr = %s, port = %d\n", K.addr, K.port);
	return;
}

item_r* sort_r(item_r* list1, item_r* list2){
	return NULL;
}
