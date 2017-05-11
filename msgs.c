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
	while (*p != '\0'){
		if(nbytes != 0)
			p++;
		nbytes = recv(socket, p, max_length, flags);
		if (nbytes <= 0) break;
		p += nbytes;
		p--;
		total_bytes += nbytes;
	}
	if(total_bytes > max_length)
		return -2; //overflow of buffer
	if(nbytes <= 0)
		return nbytes; //error or connection closed
	else
		return total_bytes;

}
