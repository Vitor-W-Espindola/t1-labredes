// Describes the operations that go underly the server application
// Works in consonance with the unix socket interface

#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 8888
#define SERVER_BUF_LEN 516
#define SERVER_MAX_CONNECTED_USERS 2
#define SERVER_MAX_PENDING_CONNECTIONS 5

struct user {
	int user_socket_fd;
	struct sockaddr_in user_addr_info;
	char nickname[];
};

struct server {
	struct user connected_users[SERVER_MAX_CONNECTED_USERS];
	int num_connected_users;
	int max_connected_users;
	int server_socket_fd;
	struct sockaddr_in server_addr_info;
};

int create_server(struct server * server);
void * server_response(void * server);
int start_server(struct server * server);

int send_response_to_client(int client_socket_fd, uint8_t response);

#endif
