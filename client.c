#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdint.h> //uint_'s
#include <errno.h>
#include "msgs.h"
#include "API.h"

#define LINE_S 100
#define COMMANDS " add, keyadd, search, delete, getname, download, quit" //Possible commands
#define DEBUG printf("aqui\n")

int fd;
struct sigaction *handler;

void test();
void usage(char*);
void notify_server(int n);
int checkPeerState();

void launch_usage(){
	printf("\x1B[31mUsage:\x1B[0m ./client <gateway_ip> <gateway_port>\n");
	exit(0);
}

int main(int argc, char* argv[]){

	char gateway_ip[20];
	int gateway_port;

	if(argc == 3){
		strcpy(gateway_ip, argv[1]);
		gateway_port = atoi(argv[2]);
	}else{
		launch_usage();
	}

	//Action of SIGINT
	handler = malloc(sizeof(handler));
    handler->sa_handler = &notify_server;
	sigaction(SIGINT, handler, NULL);

	char line[LINE_S], command[10], arg[50];

	/*Ignore SIGPIPE to prevent shutdown when writing to a closed socket*/
	if( signal(SIGPIPE,SIG_IGN) == SIG_ERR)
		exit(1);

	while(1){
	/*****************Connection to Peer*********************/
		fd =  gallery_connect(gateway_ip, gateway_port);

		switch(fd){
			case -1:
				printf("\x1B[31m[ABORTING]\x1B[0m Gateway cannot be accessed\n");
				exit(1);
				break;
			case 0:
				printf("\x1B[31m[ABORTING]\x1B[0m There are no Peers availabe\n");
				exit(0);
				break;
			default:
				printf("\x1B[32mConnection to Peer successful\x1B[0m\n");
		}
	/*****************************MENU*****************************/
		printf("---------------------------------------------------\n");
		while(1){
			//Menu message
			printf("---------------------------------------------------");
			printf("\nPlease enter your command\n");
			printf("[%s]\n\n> ", COMMANDS);

			//(Re)initializing
			strcpy(command,"");
			strcpy(arg,"");
			//Get user input
			fgets(line,LINE_S, stdin);
			sscanf(line,"%s %[^\n]", command, arg);

			//Command to add a photo to the gallery
			if(strcmp(command,"add") == 0){
				if(strcmp(arg,"") != 0){
					uint32_t photo_id = gallery_add_photo(fd, arg);
					if(photo_id){
						printf("\x1B[32mUploading successful:\x1B[0m photo_id = %u\n", photo_id);
					}else{
						printf("\x1B[31mError uploading photo!\x1B[0m\n");
					}
				}else{
					usage("add <filename>");
				}
			}
			//Command to add a keyword to a photo
			else if(strcmp(command,"keyadd") == 0){
				uint32_t photo_id = 0; char keyword[LINE_S];
				int reads = sscanf(arg, "%u %s", &photo_id, keyword);
				if(photo_id > 0 && reads == 2){
					int success = gallery_add_keyword(fd, photo_id, keyword);
					if(success == 1){
						printf("Keyword '%s' added \x1B[32msuccesfully!\x1B[0m\n", keyword);
					}else if(success == 0){
						printf("\x1B[31mCommunication error\x1B[0m\n");
					}else if(success == -1){
						printf("\x1B[31mKeyword list already full\x1B[0m\n");
					}else if(success == -2){
						printf("Photo with id = %u \x1B[31mdoesn't exist in the gallery\x1B[0m\n",photo_id);
					}else if(success == -3){
						printf("Photo with id = %u \x1B[31malready has keyword\x1B[0m: '%s'\n",photo_id, keyword);
					}
				}else{
					usage("keyadd <photo_id> <keyword>, [photo_id > 0]");
				}
			}
			//Command to search for a photo
			else if(strcmp(command, "search") == 0){
				if(strcmp(arg,"") != 0){
					uint32_t *id_photos;
					int length = gallery_search_photo(fd, arg, &id_photos);
					if(length > 0){
						printf("\x1B[32m%d photo(s) found\x1B[0m with keyword '%s'!\n", length, arg);
						printf("ID(s) of the photo(s):\n");
						for(int i = 0; i < length; i++){
							printf("---%d: %u\n", i+1, id_photos[i]);
						}
					}else if(length == 0){
						printf("\x1B[31mNo photo found\x1B[0m with keyword '%s'\n", arg);
					}else if(length == -1){
						printf("\x1B[31mCommunication error\x1B[0m\n");
					}
				}else{
					usage("search <keyword>");
				}
			}
			//Command to delete a photo
			else if(strcmp(command,"delete") == 0){
				uint32_t photo_id = 0;
				sscanf(arg, "%u", &photo_id);
				if(photo_id > 0){
					int success = gallery_delete_photo(fd, photo_id);
					if(success == 1){
						printf("Photo with id = %u deleted \x1B[32msuccesfully!\x1B[0m\n", photo_id);
					}else if(success == 0){
						printf("Photo with id = %u \x1B[31mdoesn't exist in the gallery\x1B[0m\n",photo_id);
					}
				}else{
					usage("delete <photo_id>, [photo_id > 0]");
				}
			}
			//Command to get a photo name
			else if(strcmp(command, "getname") == 0){
				uint32_t photo_id = 0;
				sscanf(arg, "%u", &photo_id);
				if(photo_id > 0){
					char *photo_name;
					int success = gallery_get_photo_name(fd, photo_id, &photo_name);
					if(success == 1){
						printf("Photo with id = %u has name: \x1B[36m%s\x1B[0m\n", photo_id, photo_name);
					}else if(success == 0){
						printf("Photo with id = %u \x1B[31mdoesn't exist in the gallery\x1B[0m\n",photo_id);
					}
				}else{
					usage("getname <photo_id>, [photo_id > 0]");
				}
			}
			//Command to get a photo
			else if(strcmp(command, "download") == 0){
				uint32_t photo_id = 0; char file_name[LINE_S];
				int reads = sscanf(arg, "%u %s", &photo_id, file_name);
				if(photo_id > 0 && reads == 2){
					int success = gallery_get_photo(fd, photo_id, file_name);
					if(success == 1){
						printf("Photo with id = '%u' downloaded \x1B[32msuccesfully!\x1B[0m to file '%s'\n", photo_id, file_name);
					}else if(success == 0){
						printf("Photo with id = %u \x1B[31mdoesn't exist in the gallery\x1B[0m\n",photo_id);
					}else if(success == -1){
						printf("\x1B[31mCommunication error\x1B[0m\n");
					}
				}else{
					usage("download <photo_id> <file_name>, [photo_id > 0]");
				}
			}
			//Commmand to quit
			else if(strcmp(command, "quit") == 0){
				printf("Closing connection.\n");
				notify_server(0);
			}
			//None of the above commands
			else{
				printf("\x1B[31mInvalid command!\x1B[0m\n");
			}
			// Sees if peer closed the connection
			int state = checkPeerState();
			if(state == -1)
				break;
			else if(state == 1){
				printf("\x1B[33mTimeout occured\x1B[0m\n");
			}
		}

	/*********************MENU[END]**********************************/
		close(fd);
		printf("Trying to connect with other peer\n");
	}
	exit(0);
}

/*******************************************************************************
	Prints the usage for each command
*******************************************************************************/
void usage(char *message){
	printf("\x1B[31mUsage: \x1B[0m%s\n", message);
}

/*******************************************************************************
	Sends a message to the peer to notify it that the client is breaking the
connection
*******************************************************************************/
void notify_server(int n) {
	stream_and_send_photo(fd, "Client exiting", -1);

	close(fd);
	free(handler);
	exit(0);
}

/*******************************************************************************
	//TODO comment this function
*******************************************************************************/
int checkPeerState(){
	if(errno == EAGAIN || errno == EWOULDBLOCK){
		//timeout occured
		errno = 0;
		return 1;
	}else if(errno == EPIPE){
		printf("Peer has disconnected\n");
		errno = 0;
		return -1;
	}else
		return 0;
}
