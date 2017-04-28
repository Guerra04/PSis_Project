//#define SOCKET_NAME "best_socket"
#define MESSAGE_LEN 100
#define KNOWN_IP "127.0.0.1"
#define KNOWN_PORT 3000

typedef struct message_gw{
	int type;
	char addr[20];
	int port;
} message_gw;

typedef struct message_tcp{
	char buffer[MESSAGE_LEN];
} message_tcp;
