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
		char *buffer;
		int type;
	};
	struct def msg;
	char frase[] = "coconibo";
	int x = 2;

	msg.type = x;
	msg.buffer = malloc(strlen(frase)*sizeof(char));
	strcpy(msg.buffer, frase);

	printf("%lu\n", sizeof(msg));
	printf("%lu\n", sizeof(struct def));

	return 0;
}
