#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h> //uint_'s
#include "msgs.h"
#include "linked_list.h"
#include "ring_list.h"

#define PORT 3000 + getpid()
#define KEYWORD_SIZE 100

#define DEBUG printf("aqui\n")
#define DEBUG_THREAD(thread) printf("%s thread = %lu\n", thread, pthread_self());

int sock_fd;
int sock_fd_gw;
struct sockaddr_in server_addr;
struct sigaction *handler;
item *photo_list = NULL;
item_r *peer_list = NULL;
//TODO verificar se ta lock em todo o lado
pthread_mutex_t photo_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t peer_lock = PTHREAD_MUTEX_INITIALIZER;

void strupr(char * line);
void *connection(void *client_fd);
void testing_comm(int fd, message_photo *msg);
int add_photo(int fd, message_photo *msg, int isPeer);
void add_keyword(int fd, message_photo *msg);
void search_photo(int fd, message_photo *msg);
void delete_photo(int fd, message_photo *msg);
void send_photo_name(int fd, message_photo *msg);
void send_photo(int fd, message_photo *msg, int isPeer, item* aux);
int search_keyword(item *photo, char *keyword);
int connect_peer(char * addr, in_port_t port);
int notify_and_recv_photos();
void register_peer(message_photo *msg);
void send_all_photos(int fd, message_photo *msg);
int notify_and_recv_photos(message_photo *msg);
void photo_replication(int fd, message_photo *msg);
void delete_peer(message_photo *msg);
int broadcastPeersAndNotify(char* message, int type, item* photo);
void printPhotos();
void printPeers();

void kill_server(int n) {
	//Message to send to gateway letting it know that this peer terminated
	if(stream_and_send_gw(sock_fd_gw, &server_addr, peer_list->K.addr, PORT, -1) == -1)
		exit(1);
	close(sock_fd);
	close(sock_fd_gw);
	pthread_mutex_lock(&photo_lock);
	list_free(photo_list);
	pthread_mutex_unlock(&photo_lock);
	free(handler);
	exit(0);
}

int main(int argc, char* argv[]){
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	int err;

	//Action of SIGINT
	handler = malloc(sizeof(handler));
    handler->sa_handler = &kill_server;
	sigaction(SIGINT, handler, NULL);
	/*******************/

/*************Communication with Gateway*****************/
	sock_fd_gw= socket(AF_INET, SOCK_DGRAM, 0);

	if (sock_fd_gw == -1){
		perror("socket: ");
		exit(-1);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(KNOWN_PORT_PEER);
	inet_aton(KNOWN_IP, &server_addr.sin_addr);
	//Sending to gatway this peer's information
	if(stream_and_send_gw(sock_fd_gw, &server_addr, "", PORT, 0) == -1)
		exit(1);

	/*****Waits for acknowledgment of gateway****/
	//sets timeout of recv(...)
	set_recv_timeout(sock_fd_gw, 5, 0);
	//Waits for list of peers
	if(recv_ring_udp(sock_fd_gw, &peer_list) == -1){
		if(errno == EAGAIN || errno == EWOULDBLOCK){
			//timeout occured
			printf("[ABORTING] The Gateway is not online\n");
			exit(1);
		}
		perror("Receiving from gateway");
		exit(1);
	}
	//Resets the timeout for the default
	reset_recv_timeout(sock_fd_gw);

	//Puts root of peer_list referencing this peer's identification element
	//(when the gateway adds this peer to the list, it adds it in the end)
	peer_list = peer_list->prev;
	//Prints peer list
	printPeers();

	//Informs the other peers that this peer have entered in the system
	message_photo * msg = malloc(sizeof(message_photo));
	if(notify_and_recv_photos(msg) == -1)
		exit(1);
	free(msg);
	printPhotos();


/*****************************SOCKET TCP*****************************/

	sock_fd= socket(AF_INET, SOCK_STREAM, 0);
	socklen_t size_addr;

	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(PORT);
	local_addr.sin_addr.s_addr= INADDR_ANY;
	err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

	//nanosleep of 10ms
	struct timespec sleep;
	sleep.tv_sec = 0;			/* seconds */
	sleep.tv_nsec = 10000000;   /* nanoseconds */

	listen(sock_fd, 5);
	int client_fd;
	while(1){
		client_fd = accept(sock_fd, (struct sockaddr *) & client_addr, &size_addr);
		pthread_t thread_id;

		if(pthread_create(&thread_id, NULL, connection, &client_fd) != 0){
			perror("Creating thread: ");
		}

		/*nanosleep of 10ms to ensure that if two accepts are made consecutively
		the client_fd isn't changed while a thread is acquiring the fd attributed to him*/
		nanosleep(&sleep, NULL);
	}
	close(sock_fd);
	exit(0);
}

/*****************************************************************************
 * Routine of the threads that handle new connections
 ****************************************************************************/
void *connection(void *client_fd){
	//Value copied becaus client_fd will be altered
	int fd = *(int*)client_fd;
	printf("---------------------------------------------------\n");
	message_photo * msg = malloc(sizeof(message_photo));
	while(recv_and_unstream_photo(fd, msg) > 0){//CHANGED
		switch(msg->type){
			case 1:
				add_photo(fd, msg, 0);
				printPhotos();
				break;
			case 2:
				add_keyword(fd, msg);
				printPhotos();
				break;
			case 3:
				search_photo(fd, msg);
				break;
			case 4:
				delete_photo(fd, msg);
				printPhotos();
				break;
			case 5:
				send_photo_name(fd, msg);
				break;
			case 6:
				send_photo(fd, msg, 0, NULL);
				break;
			case -1:
				printf("\x1B[31mClient broke connection!\x1B[0m\n");
				close(fd);
				free(msg);
				pthread_exit(NULL);
				break;
			case 7:
				register_peer(msg);
				printPeers();
				break;
			case 8:
				register_peer(msg);
				send_all_photos(fd, msg);
				printPeers();
				break;
			case 9:
				photo_replication(fd, msg);
				printPhotos();
				break;
			case 10:
				add_keyword(fd, msg);
				printPhotos();
				break;
			case 11:
				delete_photo(fd, msg);
				printPhotos();
				break;
			case 12:
				delete_peer(msg);
				printPeers();
				break;
			default:
				strcpy(msg->buffer,"Type of message undefined!");
				printf("%s\n", msg->buffer);
		}
	}
	printf("---------------------------------------------------\n");
	free(msg);
	close(fd);
	pthread_exit(NULL);
	return NULL;
}

/*****************************************************************************
 * Adds photo to the system. If isPeer=0, this peer is receiving a request from
 * the client to receive the photo. If isPeer!=0, it's receiving a replication of
 * a photo from other peer.
 * Returns: -1 		- error
 * 			photo_id - success
 ****************************************************************************/
int add_photo(int fd, message_photo *msg, int isPeer){
	char name[MAX_SIZE];
	long size=0;
	uint32_t id=0;
	int n_keywords = 0;
	char photo_name[MAX_SIZE];
	//Buffer changes whether is receiving from a client or a peer
	if(isPeer)
		sscanf(msg->buffer,"%[^,],%lu.%u.%d", name, &size, &id, &n_keywords);
	else
		sscanf(msg->buffer,"%[^,],%lu", name, &size);

	if(!isPeer){
		//Asks Gateway for a new id
		if(stream_and_send_gw(sock_fd_gw, &server_addr, "", PORT, 1) == -1)
			exit(1);

		//Receives id from the gateway
		socklen_t size_addr = sizeof(server_addr);
		if(recvfrom(sock_fd_gw, &id, sizeof(uint32_t), 0, (struct sockaddr *) &server_addr, &size_addr) == -1){
			perror("wtf: ");
			exit(1);
		}
	}

	//Saving photo characteristics in list
	data photo;
	photo = set_data(name, id);
	if(isPeer && n_keywords > 0){
		char *keywords = malloc(MAX_KEYWORDS*SIZE*sizeof(char));
		memset(keywords, '\0', MAX_KEYWORDS*SIZE*sizeof(char));
		if(recv_all(fd, keywords, n_keywords*SIZE*sizeof(char), 0) == -1)
			exit(1);
		//separate and store keywords
		char *keyword;
		while((keyword = strsep(&keywords, " "))){
			if(strcmp(keyword, "") != 0){
				strcpy(photo.keyword[photo.n_keywords],keyword);
				photo.n_keywords++;
			}
		}
		free(keywords);
	}
	pthread_mutex_lock(&photo_lock);
	list_insert(&photo_list, photo);
	pthread_mutex_unlock(&photo_lock);

	//Receive photo
	char *bytestream = malloc(size*sizeof(char));
	if(recv_all(fd, bytestream, size, 0) <= 0){
		perror("Receiving: ");
		return -1;
	}

	//Saving photo in disk
	sprintf(photo_name, "%u", id);
	FILE *fp;
	if((fp = fopen( photo_name, "wb")) == NULL){
		perror("Opening file to write");
		exit(-1);
	}
	fwrite(bytestream, size, 1, fp);
	fclose(fp);
	free(bytestream);

	if(!isPeer){
		//Sending photo id to client
		if( send_all(fd, &id, sizeof(uint32_t), 0) == -1){
			perror("Sending id: ");
			return -1;
		}

		//Broadcast (Replication)
		item *send = malloc(sizeof(item));
		send->K = photo;
		if(broadcastPeersAndNotify("", 9, send))
			printPeers();

		free(send);
	}

	return id;
}

/*****************************************************************************
 * Adds a keyword to a photo in the system.
 ****************************************************************************/
void add_keyword(int fd, message_photo *msg){
	uint32_t id;
	char keyword[KEYWORD_SIZE];
	int success;
	//Retrieves photo id and keyword
	sscanf(msg->buffer, "%u.%s", &id, keyword);
	data K;
	K.id = id;
	pthread_mutex_lock(&photo_lock);
	item* aux = list_search(&photo_list, K);
	pthread_mutex_unlock(&photo_lock);

	if(aux == NULL){ //photo with sent id doesn't exist
		success = -2;
	}else{
		if(aux->K.n_keywords < MAX_KEYWORDS){
			pthread_mutex_lock(&photo_lock);
			strcpy(aux->K.keyword[aux->K.n_keywords++],keyword);
			pthread_mutex_unlock(&photo_lock);
			success = 1;
		}else{ //keyword list already full
			success = -1;
		}
	}

	//Just answers and broadcasts if instruction comes from a client
	if(msg->type != 10){

		if( send_all(fd, &success, sizeof(int), 0) == -1){
			perror("Sending: ");
		}
		if(success == 1){ //Broadcasts if keyword was succesfully inserted
			if(broadcastPeersAndNotify(msg->buffer, 10, NULL))
				printPeers();
		}
	}

	return;
}

/*****************************************************************************
 * Searches for all the photos with the provided keyword in the system.
 ****************************************************************************/
void search_photo(int fd, message_photo *msg){
	char keyword[KEYWORD_SIZE];
	//Retrieves keyword
	strcpy(keyword, msg->buffer);

	int n_photos = 0;
	item *list = list_init(); //list to store photos
	pthread_mutex_lock(&photo_lock);
	item *photo = photo_list;
	//Searches keyword in all photos
	while(photo != NULL){
		if(search_keyword(photo, keyword)){
			data K = set_data("", photo->K.id);
			list_insert(&list, K);
			n_photos++;
		}
		photo = photo->next;
	}
	pthread_mutex_unlock(&photo_lock);

	//send length of list
	if( send_all(fd, &n_photos, sizeof(int), 0) == -1){
		perror("Sending: ");
		return;
	}

	if(n_photos == 0){
		return;
	}
	//transform list into an array
	uint32_t *id_photos = malloc(n_photos * sizeof(uint32_t));
	for(int i = 0; i < n_photos; i++){
		item *aux = list_first(&list);
		id_photos[i] = aux->K.id;
		free(aux);
	}

	//send array
	if( send_all(fd, id_photos, n_photos * sizeof(uint32_t), 0) == -1){
		perror("Sending: ");
	}
	return;
}

/*****************************************************************************
 * Deletes a photo from the system.
 ****************************************************************************/
void delete_photo(int fd, message_photo *msg){
	uint32_t id;
	//Retrieves photo id
	sscanf(msg->buffer, "%u", &id);
	data K;
	K.id = id;

	pthread_mutex_lock(&photo_lock);
	int found = list_remove(&photo_list, K);
	pthread_mutex_unlock(&photo_lock);
	if(found != 0){ //photo with sent id exists
		//Removes photo from disk
		char file_name[50];
		sprintf(file_name,"%u", id);
		unlink(file_name);
	}

	//Just answers and broadcasts if instruction comes from a client
	if(msg->type != 11){
		//Says to client if photo was found (and deleted) or not
		if( send_all(fd, &found, sizeof(int), 0) == -1){
			perror("Sending: ");
		}
		if(found != 0){ //Broadcasts if the photo to delete was found
			if(broadcastPeersAndNotify(msg->buffer, 11, NULL))
				printPeers();
		}
	}

	return;
}

/*****************************************************************************
 * Sends the name of a photo.
 ****************************************************************************/
void send_photo_name(int fd, message_photo *msg){
	uint32_t id;
	//Retrieves photo id
	sscanf(msg->buffer, "%u", &id);
	data K = set_data("", id); //aux data to search

	pthread_mutex_lock(&photo_lock);
	item *aux = list_search(&photo_list, K);
	pthread_mutex_unlock(&photo_lock);

	int length = 0;
	if(aux != NULL){ //photo exists
		length = strlen(aux->K.name)+1; //strlen doesn't count with '\0'
	}
	//send length of the photo name
	if( send_all(fd, &length, sizeof(int), 0) == -1){
		perror("Sending: ");
	}

	//send photo name
	if(length > 0){
		if( send_all(fd, aux->K.name, length, 0) == -1){
			perror("Sending: ");
		}
	}
	return;
}

/*****************************************************************************
 * Sends a photo to a client, if isPeer=0, or a peer, if isPeer!=0.
 ****************************************************************************/
void send_photo(int fd, message_photo *msg, int isPeer, item* aux){

	uint32_t id;
	data photo;
	/*
	 * Peer already has the photo to send, doesn't have to search
	 */
	if(isPeer){
		photo = aux->K;
	}else{
		sscanf(msg->buffer, "%u", &id);
		data K = set_data("", id); //aux data to search

		pthread_mutex_lock(&photo_lock);
		aux = list_search(&photo_list, K);
		pthread_mutex_unlock(&photo_lock);
		if(aux != NULL)
			photo = aux->K;
	}

	FILE *fp;
	long size = 0;
	//Opens image as a binary file and acquires its size
	if(aux != NULL){
		char file_name[MAX_SIZE];
		sprintf(file_name, "%u", photo.id);

		if((fp = fopen(file_name, "rb")) == NULL){
			//Invalid filename
			perror("Filename: ");
		}
		fseek(fp, 0, SEEK_END); // jumps to the end of the file
		size = ftell(fp);  // gets the current byte offset in the file
		rewind(fp);
	}

	if(isPeer){
		char message[MAX_SIZE];
		//message: photo name, photo size, photo id
		sprintf(message, "%s,%lu.%u.%d", photo.name, size, photo.id, photo.n_keywords);
		if(stream_and_send_photo(fd, message, 1) == -1)
			exit(1);
		if(photo.n_keywords > 0){
			//Concatenate keywords into one string
			char keywords[aux->K.n_keywords*SIZE];
			memset(keywords, '\0', aux->K.n_keywords*SIZE);
			for(int i = 0; i < aux->K.n_keywords;i++){
				strcat(keywords, aux->K.keyword[i]);
				strcat(keywords, " ");
			}
			//send keywords
			if(send_all(fd, keywords, aux->K.n_keywords*SIZE, 0) == -1)
				exit(1);
		}
	}else{
		//Send size of the photo
		if(send_all(fd, &size, sizeof(long), 0) == -1){
			perror("Sending: ");
		}
	}
	//Only sends photo if size isn't zero
	if(size != 0){
		char *photo_stream = malloc((size)*sizeof(char));
		fread(photo_stream, size, 1, fp); //reads the whole file at once
		fclose(fp);
		//Send photo
		if(send_all(fd, photo_stream, size, 0) == -1){
			perror("Sending: ");
		}
		free(photo_stream);
	}
}

/*******************************************************************************
Returns:
	- 1: if keyword exists in photo
	- 0: otherwise
*******************************************************************************/
int search_keyword(item *photo, char *keyword){
	for(int i = 0; i < photo->K.n_keywords; i++){
		if(strcmp(photo->K.keyword[i], keyword) == 0)
			return 1;
	}
	return 0;
}

/*******************************************************************************
Creates a TCP connection between peers.
Returns:
	- socket file descriptor on success.
	- -1: on error
*******************************************************************************/
int connect_peer(char * addr, in_port_t port){
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_fd == -1){
		perror("socket: ");
		exit(-1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port= htons(port);
	inet_aton(addr, &server_addr.sin_addr);

	if( -1 == connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr))){
		perror("Connecting to Peer: ");
		exit(-1);
	}

	return sock_fd;
}

/*****************************************************************************
 * Notifies other peers, which are not the calling one, about this identity,
 * and asks to one of them for the photos already in the system.
 * Returns: 0 - success
 * 			-1 - error
 ****************************************************************************/
int notify_and_recv_photos(message_photo *msg){
	if(ring_count(peer_list) != 1){
		//buffer that has this peer's address and port
		char buffer[MAX_SIZE];
		sprintf(buffer, "%s,%d", peer_list->K.addr, peer_list->K.port);

		item_r *aux = peer_list->next;
		int has_photos = 0; //verify if peer has received the photos
		while(aux != peer_list){
			int p2p_sock = connect_peer(aux->K.addr, aux->K.port);
			if(!has_photos){// informs a peer that this one entered the system
							// and receives all the photos in the system from that peer
				if(stream_and_send_photo(p2p_sock, buffer, 8) == -1){
					return -1;
				}
				//First receives the size of the photo list
				int size=0;
				if(recv_all(p2p_sock, &size, sizeof(int), 0) == -1){
					perror("Receive size: ");
					return -1;
				}
				//Then receives all photos, one by one
				if(size != 0){
					for(int i = 0; i < size; i++){
						recv_and_unstream_photo(p2p_sock, msg);
						add_photo(p2p_sock, msg, 1);
					}
				}
				has_photos = 1;
			}else{// just informs other peers that this entered the system
				if(stream_and_send_photo(p2p_sock, buffer, 7) == -1){
					return -1;
				}
			}
			aux = aux->next;
			close(p2p_sock);
		}
	}
	return 0;
}

/*******************************************************************************
Appends new peer in the peer list
*******************************************************************************/
void register_peer(message_photo *msg){
	char addr[MAX_SIZE];
	int port = 0;
	sscanf(msg->buffer, "%[^,],%d", addr, &port);
	data_r K = set_data_r(addr, port);
	pthread_mutex_lock(&peer_lock);
	ring_append(&peer_list, K);
	pthread_mutex_unlock(&peer_lock);
	return;
}

/*******************************************************************************
Sends all the photos to the new peer
*******************************************************************************/
void send_all_photos(int fd, message_photo *msg){
	pthread_mutex_lock(&photo_lock);
	//Send size of photo_list
	int size = list_count(photo_list);
	if(send_all(fd, &size, sizeof(int), 0) == -1){
		perror("Sending list size:");
		exit(1);
	}
	//Sends photo_list, photo by photo
	item *aux = photo_list;
	while(aux != NULL){
		sprintf(msg->buffer, "%u", aux->K.id);
		send_photo(fd, msg, 1, aux);
		aux = aux->next;
	}
	pthread_mutex_unlock(&photo_lock);
	return;
}

/*******************************************************************************
* Receives replicated photo from peer.
*******************************************************************************/
void photo_replication(int fd, message_photo *msg){
	recv_and_unstream_photo(fd, msg);
	add_photo(fd, msg, 1);
}

/*******************************************************************************
* Deletes a peer from peer_list
*******************************************************************************/
void delete_peer(message_photo *msg){
	data_r K;
	sscanf(msg->buffer, "%[^,],%u", K.addr, &K.port);
	pthread_mutex_lock(&peer_lock);
	ring_remove(&peer_list, K);
	pthread_mutex_unlock(&peer_lock);
	return;
}

/*******************************************************************************
* Boradcasts to other peers, and notifies gateway if it finds a peer offline.
* Returns: number of offline peers
*******************************************************************************/
int broadcastPeersAndNotify(char* message, int type, item* photo){
	int offline_peers = 0;
	pthread_mutex_lock(&peer_lock);
	item_r *aux = peer_list->next;
	while(aux != peer_list){
		int p2p_fd = 0;
		if( (p2p_fd = isOnline(aux->K.addr, aux->K.port)) ){
			stream_and_send_photo(p2p_fd, message, type);
			if(photo != NULL){
				//Send photo
	 			send_photo(p2p_fd, NULL, 1, photo);
			}
			close(p2p_fd);
		}else{
			// Warns Gateway about peer's death
			char peer_id[30];
			sprintf(peer_id, "%s,%u", aux->K.addr, aux->K.port);
			if(stream_and_send_gw(sock_fd_gw, &server_addr, peer_id, PORT, -2) == -1)
				exit(1);
		}
		aux = aux->next;
		// removes peer from list if it's not online
		if(p2p_fd == 0){
			ring_remove(&peer_list, aux->prev->K);
			offline_peers++;
		}
	}
	pthread_mutex_unlock(&peer_lock);
	return offline_peers;
}
/*******************************************************************************
* Prints photo list
*******************************************************************************/
void printPhotos(){
	printf("\x1B[32m[Updated!]\n");
	printf("\x1B[36m===========Photos==========\n");
	pthread_mutex_lock(&photo_lock);
	list_print(photo_list);
	pthread_mutex_unlock(&photo_lock);
	printf("===========================\x1B[0m\n");
}
/*******************************************************************************
* Prints peer list
*******************************************************************************/
void printPeers(){
	printf("\x1B[32m[Updated!]\n");
	printf("\x1B[33m*********Peers list***********\n");
	pthread_mutex_lock(&peer_lock);
	ring_print(peer_list);
	pthread_mutex_unlock(&peer_lock);
	printf("*****************************\x1B[0m\n");
}
