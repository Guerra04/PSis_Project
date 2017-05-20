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
	char name[MAX_SIZE];
	char ext[10];
	long size=0;

	sscanf("thisimage.jpg50000","%[^.].%[^01233456789]%lu", name, ext, &size);
	printf("%s %s %lu\n", name, ext, size);

	return 0;
}
