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
#define LINE_S 100
#define COMMANDS " add, keyadd, search, delete, getname, download"
#define DEBUG printf("aqui\n")

int fd;
message_photo * msg;
struct sigaction *handler;

void test();
void usage(char*);
void kill_server(int n);

int main(int argc, char* argv[]){

	/*struct sockaddr_in client_gw_addr;
	int nbytes;
	int err;*/
	char line[LINE_S], command[10], arg[50];

/*****************Connection to Peer*********************/
	fd =  gallery_connect(KNOWN_IP, KNOWN_PORT_CLIENT);

	switch(fd){
		case -1:
		printf("[ABORTING] Gateway cannot be accessed\n");
		exit(1);
		break;
		case 0:
		printf("[ABORTING] There are no Peers availabe\n");
		exit(0);
		break;
		default:
		printf("Connection to Peer successful\n");
	}
/*****************************MENU*****************************/
	printf("---------------------------------------------------\n");
	while(1){
		//Menu message
		printf("Please enter your command\n");
		printf("[%s]\n\n", COMMANDS);

		//(Re)initializing
		strcpy(command,"");
		strcpy(arg,"");
		//Get user input
		fgets(line,LINE_S, stdin);
		sscanf(line,"%s %s", command, arg);//TODO maybe scan for garbage

		//[HIDDEN] é só pra nós testarmos
		if(strcmp(command,"test") == 0){
			test();
		}
		//Command to add a photo to the gallery
		else if(strcmp(command,"add") == 0){
			if(strcmp(arg,"") != 0){
				gallery_add_photo(fd, arg);
			}else{
				usage("add <filename>");
			}
		}
		//None of the above commands
		else{
			printf("Invalid command!\n");
		}

	}

/*********************MENU[END]**********************************/
	close(fd);
	exit(0);
}

void test(){
	/*************Communication**************/
	char *stream = malloc(sizeof(message_photo));
	message_photo *msg = malloc(sizeof(message_photo));
	memset(msg->buffer, 0, MAX_SIZE);
	fgets(msg->buffer,MAX_SIZE, stdin);
	memcpy(stream, msg, sizeof(message_photo));
	send(fd, stream, sizeof(message_photo), 0);
	while(recv(fd, stream, sizeof(message_photo), 0) != EOF){
		memcpy(msg, stream, sizeof(message_photo));
		printf("%s\n", msg->buffer);
		memset(msg->buffer, 0, MAX_SIZE);
		fgets(msg->buffer,MAX_SIZE, stdin);
		memcpy(stream, msg, sizeof(message_photo));
		send(fd, msg, sizeof(message_photo), 0);
	}
	/********Communication [END]*****************/
}

void usage(char *message){
	printf("Usage: %s\n", message);
}

void kill_server(int n) {
	close(fd);
	free(msg);
	free(handler);
	exit(0);
}
