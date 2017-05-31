#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include "msgs.h"
#include "ring_list.h"

#define DEBUG printf("aqui\n")

int sock_fd_peer;
int sock_fd_client;
struct sigaction *handler;
item_r* peer_list;
int sigint = 0;
pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
uint32_t photo_id = 0;

void kill_server(int n) {
	sigint = 1;
}

void *connection_peer(void *args);
void *connection_client(void *args);
void broadcastPeers(char* message, int type, data_r *exc);
void printPeers();

int main(){
	//initializing peer list
	peer_list = ring_init();

	//Action of SIGINT
	handler = malloc(sizeof(handler));
    handler->sa_handler = &kill_server;
	handler->sa_flags = SA_SIGINFO;
	sigaction(SIGINT, handler, NULL);
	/*******************/

	pthread_t thread_peer;
	pthread_t thread_client;
	//Thread that takes care of peers
	if(pthread_create(&thread_peer, NULL, connection_peer, NULL) != 0){
		perror("Fail creating thread_peer\n");
	}
	//Thread that takes care of clients
	if(pthread_create(&thread_client, NULL, connection_client, NULL) != 0){
		perror("Fail creating thread_client\n");
	}
	/*When one of the 3 threads handles the SIG_INT
	* the main one closes all connections and frees all alocated memory*/
	while(!sigint);

	ring_free(peer_list);
	close(sock_fd_peer);
	close(sock_fd_client);
	free(handler);

	exit(0);
}
/******************************************************************************
*Function executed by the thread that handles the peers.
*
******************************************************************************/
void *connection_peer(void *args){
	struct sockaddr_in local_addr;
	struct sockaddr_in peer_addr;
	//Creation of socket
	sock_fd_peer = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd_peer == -1){
		perror("socket: ");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(KNOWN_PORT_PEER);
	local_addr.sin_addr.s_addr= INADDR_ANY;

	int err = bind(sock_fd_peer, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind: ");
		exit(-1);
	}
	printf(" socket created and binded \n Ready to receive messages\n");

	message_gw *buff = malloc(sizeof(message_gw));
	while(1){
		//Receives and unstreams message from peer
		if(recv_and_unstream_gw(sock_fd_peer, &peer_addr, buff) == -1)
			exit(1);
		printf("buff: type = %d\n", buff->type);
		if(buff->type == 0){
			//Saves new peer in list
			data_r K;
			K.port = buff->port;
			strcpy(K.addr, inet_ntoa(peer_addr.sin_addr));
			pthread_mutex_lock(&list_lock);
			ring_append(&peer_list, K);
			pthread_mutex_unlock(&list_lock);
			printf("Server %s with port %d \x1B[32madded to list\x1B[0m\n", K.addr, K.port);
			//Sends peer_list to new_peer
			if( send_ring_udp(sock_fd_peer, &peer_addr, peer_list) == -1)
				exit(1);
			//Prints peer list
			printPeers();
		}else if(buff->type == -1){
			//Removes peer that sent the message from the list
			data_r K;
			K.port = buff->port;
			strcpy(K.addr, inet_ntoa(peer_addr.sin_addr));
			pthread_mutex_lock(&list_lock);
			ring_remove(&peer_list, K);
			pthread_mutex_unlock(&list_lock);
			printf("Server %s with port %d r\x1B[31mremoved from list\x1B[0m\n", K.addr, K.port);
			//Prints peer list
			printPeers();
			// Broadcast to all peers
			char message[30];
			sprintf(message, "%s,%u" , K.addr, K.port);
			broadcastPeers(message, 12, NULL);
		}else if(buff->type == -2){
			//Removes an exiting peer from the list (not the one that sent the message)
			data_r K;
			sscanf(buff->addr, "%[^,],%u", K.addr, &K.port);
			pthread_mutex_lock(&list_lock);
			ring_remove(&peer_list, K);
			pthread_mutex_unlock(&list_lock);
			printf("Server %s with port %d r\x1B[31mremoved from list\x1B[0m\n", K.addr, K.port);
			//Prints peer list
			printPeers();
			// Broadcast to other peers (excluding the one who sent the message)
			K.port = buff->port;
			strcpy(K.addr, inet_ntoa(peer_addr.sin_addr));
			broadcastPeers(buff->addr, 12, &K);
		}else if(buff->type == 1){
			++photo_id;
			sendto(sock_fd_peer, &photo_id, sizeof(uint32_t), 0,
				(const struct sockaddr *) &peer_addr, sizeof(peer_addr));
		}else{
			printf("That's the wrong number\n");
		}
	}
}

void *connection_client(void *args){
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;

	sock_fd_client = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd_client == -1){
		perror("socket: ");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(KNOWN_PORT_CLIENT);
	local_addr.sin_addr.s_addr= INADDR_ANY;

	int err = bind(sock_fd_client, (struct sockaddr *)&local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}
	printf(" socket created and binded \n Ready to receive messages\n");

	message_gw *buff = malloc(sizeof(message_gw));

	while(1){
		if(recv_and_unstream_gw(sock_fd_client, &client_addr, buff) == -1)
			exit(1);

		int peer_found = 0;
		//Searches for first online peer (more than one could have crashed)
		while(!peer_found){
			pthread_mutex_lock(&list_lock);
			item_r* aux = ring_first(&peer_list);
			pthread_mutex_unlock(&list_lock);
			if(aux != NULL){
				// Checks if peer is still online, could have crashed
				int fd_peer = 0;
				if(!(fd_peer = isOnline(aux->K.addr, aux->K.port))){
					// Broadcast to all peers that this one is dead
					char message[30];
					sprintf(message, "%s,%u" , aux->K.addr, aux->K.port);
					broadcastPeers(message, 12, NULL);
					printPeers();
					free(aux);
					continue;
				}
				peer_found = 1;
				close(fd_peer);
				//Puts peer identification into struct to send to client
				buff->type = 0;
				strcpy(buff->addr, aux->K.addr);
				buff->port = aux->K.port;
				pthread_mutex_lock(&list_lock);
				ring_append(&peer_list, aux->K);
				pthread_mutex_unlock(&list_lock);
			}else{
				buff->type = 1;
				break;
			}
		}
		//printf("%p\n", aux);
		if(stream_and_send_gw(sock_fd_client, &client_addr, buff->addr,
			buff->port, buff->type) == -1)
				exit(1);
		if(buff->type == 0){
			printf("Client sent to communicate with server %s, port %d\n",
				buff->addr, buff->port);
		}
	}
}

void broadcastPeers(char* message, int type, data_r *exc){
	pthread_mutex_lock(&list_lock);
	item_r *aux = peer_list;
	if(peer_list != NULL){
		do{	// excludes the one that sent the message (if there is any)
			if(exc == NULL || !equal_data_r(aux->K, *exc)){
				int g2p_fd = 0;
				if( (g2p_fd = isOnline(aux->K.addr, aux->K.port)) ){
					stream_and_send_photo(g2p_fd, message, type);
					close(g2p_fd);
				}
			aux = aux->next;

			}
		}while(aux != peer_list);
	}
	pthread_mutex_unlock(&list_lock);
	return;
}

void printPeers(){
	printf("[Updated!]\n");
	printf("*********Peers list***********\n");
	pthread_mutex_lock(&list_lock);
	ring_print(peer_list);
	pthread_mutex_unlock(&list_lock);
	printf("*****************************\n");
}

/* if in the process other peer is offline, gateway will receive
 * a message that warns him of it, from the same peer that sent
 * this message, so we don't have to exclude the offline peer right away */
 //TODO tirar comment ^
