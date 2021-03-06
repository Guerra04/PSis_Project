#define MAX_SIZE 100

#include "ring_list.h"
#include "linked_list.h"

//gateway
//type 1 - fetch photo id
//type 0 - add peer
//type -1 - delete this peer
//type -2 - delete  peer
//client
//type 0 - ok
//type 1 - no peers available
typedef struct message_gw{
	int type;
	char addr[30];
	int port;
} message_gw;

/**************************************
* Message exchanged between a client
* using the API and a peer
*	# type:
*	0 - debug (strup)
*	1 - add photo
*   2 - add keyword
*	3 - search photo
*	4 - delete photo
*	5 - get photo name
*	6 - get photo (download)
*  -1 - exiting
*********P2P types**********
*	7 - recv new peer info
*	8 - recv new peer info and send photo_list+photos
*	9 - add photo (replication)
*  10 - add keyword (replication)
*  11 - delete photo (replication)
*  -2 - shutdown system (gateway offline)
********GW to peer************
*  12 - delete peer in buffer
************************************/
typedef struct message_photo{
	char buffer[MAX_SIZE];
	int type;
} message_photo;

int send_all(int socket, const void *buffer, size_t length, int flags);

int recv_all(int socket, void *buffer, size_t max_length, int flags);

int stream_and_send_gw(int fd,struct sockaddr_in *other_addr, char *addr, in_port_t port, int type);

int stream_and_send_photo(int fd, const char *buffer, int type);

void set_recv_timeout(int sock_fd, int secs, int usecs);

void reset_recv_timeout(int sock_fd);

int recv_and_unstream_gw(int sock_fd,struct sockaddr_in *other_addr, message_gw *buff);

int recv_and_unstream_photo(int sock_fd, message_photo *buff);

int recv_ring_udp(int sock_fd, item_r **peer_list);

int send_ring_udp(int sock_fd, struct sockaddr_in* other_addr, item_r *peer_list);

int recv_list_tcp(int sock_fd, item** list);

int send_list_tcp(int sock_fd, item *photo_list);

int isOnline(char * host, in_port_t port);
