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
#define SIZE 100

int main(){
	struct def{
		char buffer[SIZE];
		int type;
	};
	struct def *msg = malloc(sizeof(struct def));
	printf("%lu\n", sizeof(msg));
	printf("%lu\n", sizeof(*msg));

	char *stream = malloc(SIZE*sizeof(char)+sizeof(int));
	printf("%lu\n", sizeof(long));
	printf("%lu\n", sizeof(struct def));

	return 0;
}
