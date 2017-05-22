//#define SOCKET_NAME "best_socket"
#define MAX_SIZE 100
#define KNOWN_IP "127.0.0.1"
#define KNOWN_PORT_PEER 3000
#define KNOWN_PORT_CLIENT 3001

//server
//type 0 - thread active
//type -1 - thread delete
//client
//type 0 - ok
//type 1 - no peers available
typedef struct message_gw{
	int type;
	char addr[20];
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
************************************/
typedef struct message_photo{
	char buffer[MAX_SIZE];
	int type;
} message_photo;

int send_all(int socket, const void *buffer, size_t length, int flags);

int recv_all(int socket, void *buffer, size_t max_length, int flags);
