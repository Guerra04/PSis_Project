//#define SOCKET_NAME "best_socket"
#define MESSAGE_LEN 100
#define KNOWN_IP "127.0.0.1"
#define KNOWN_PORT_PEER 3000
#define KNOWN_PORT_CLIENT 3001

//server
//type 0 - thread active
//type 1 - thread delete
//client
//type 0 - ok
//type 1 - no peers available
typedef struct message_gw{
	int type;
	char addr[20];
	int port;
} message_gw;

typedef struct message_tcp{
	char buffer[MESSAGE_LEN];
} message_tcp;
