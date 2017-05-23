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
#include "msgs.h"
#include "gallery.h"
#define LINE_S 100
#define COMMANDS " add, keyadd, search, delete, getname, download, quit"
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
		printf("---------------------------------------------------");
		printf("\nPlease enter your command\n");
		printf("[%s]\n\n", COMMANDS);

		//(Re)initializing
		strcpy(command,"");
		strcpy(arg,"");
		//Get user input
		fgets(line,LINE_S, stdin);
		sscanf(line,"%s %[^\n]", command, arg);//TODO maybe scan for garbage

		//[HIDDEN] é só pra nós testarmos
		if(strcmp(command,"test") == 0){
			test();
		}
		//Command to add a photo to the gallery
		else if(strcmp(command,"add") == 0){
			if(strcmp(arg,"") != 0){
				uint32_t photo_id = gallery_add_photo(fd, arg);
				if(photo_id){
					printf("\x1B[32mUploading successful:\x1B[0m photo_id = %u\n", photo_id);
				}else{
					printf("Error uploading photo!\n");
				}
			}else{
				usage("add <filename>");
			}
		}
		//Command to add a keyword to a photo
		else if(strcmp(command,"keyadd") == 0){
			uint32_t photo_id = 0; char keyword[LINE_S];
			sscanf(arg, "%u %s", &photo_id, keyword);
			if(photo_id > 0 && strcmp(keyword, "") != 0){
				int success = gallery_add_keyword(fd, photo_id, keyword);
				if(success == 1){
					printf("Keyword '%s' added \x1B[32msuccesfully!\x1B[0m\n", keyword);
				}else if(success == 0){
					printf("Communication error\n");
				}else if(success == -1){
					printf("Keyword list already full\n");
				}else if(success == -2){
					printf("Photo with id = %u \x1B[31mdoesn't exist in the gallery\x1B[0m\n",photo_id);
				}
			}else{
				usage("keyadd <photo_id> <keyword>, [photo_id > 0]");
			}
		}
		//Command to search for a photo
		//TODO meter cores :)
		else if(strcmp(command, "search") == 0){
			if(strcmp(arg,"") != 0){
				uint32_t *id_photos;
				int length = gallery_search_photo(fd, arg, &id_photos);
				if(length > 0){
					printf("%d photo(s) found with keyword '%s'!\n", length, arg);
					printf("ID(s) of the photo(s):\n");
					for(int i = 0; i < length; i++){
						printf("---%d: %u\n", i+1, id_photos[i]);
					}
				}else if(length == 0){
					printf("No photo found with keyword '%s'\n", arg);
				}else if(length == -1){
					printf("Communication error\n");
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
					printf("Photo with id = %u has name: %s\n", photo_id, photo_name);
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
			sscanf(arg, "%u %s", &photo_id, file_name);
			if(photo_id > 0 && strcmp(file_name, "") != 0){
				int success = gallery_get_photo(fd, photo_id, file_name);
				if(success == 1){
					printf("Photo with id = '%u' downloaded \x1B[32msuccesfully!\x1B[0m to file '%s'\n", photo_id, file_name);
				}else if(success == 0){
					printf("Photo with id = %u \x1B[31mdoesn't exist in the gallery\x1B[0m\n",photo_id);
				}else if(success == -1){
					printf("Communication error\n");
				}
			}else{
				usage("downlaod <photo_id> <file_name>, [photo_id > 0]");
			}
		}
		//Commmand to quit
		else if(strcmp(command, "quit") == 0){
			printf("Bye bye!\n"); //TODO por frase bacana
			break;
		}
		//None of the above commands
		else{
			printf("\x1B[31mInvalid command!\x1B[0m\n");
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
	printf("\x1B[31mUsage: \x1B[0m%s\n", message);
}

void kill_server(int n) {
	close(fd);
	free(msg);
	free(handler);
	exit(0);
}
