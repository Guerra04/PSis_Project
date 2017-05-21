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
	char line[MAX_SIZE];
	char c[3]={0,0,0};

	fgets(line,MAX_SIZE, stdin);
	sscanf(line,"%c %c %c", &c[0], &c[1], &c[2]);
	printf("%d %d %d\n",  c[0], (unsigned char)c[1], c[2]);

	return 0;
}
