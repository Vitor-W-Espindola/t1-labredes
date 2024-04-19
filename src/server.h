// Describes the operations that go underly the server application
// Works in consonance with the unix socket interface

#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include "rtlp.h"

#define SERVER_PORT 8888
#define SERVER_BUF_LEN 644
#define SERVER_MAX_CONNECTED_USERS 3
#define SERVER_MAX_PENDING_CONNECTIONS 5
#define SERVER_NICKNAME_LEN 16

struct user {
	struct sockaddr_in user_addr_info;
	int user_addr_info_length;
	char nickname[SERVER_NICKNAME_LEN];
	uint8_t allow_transfer;
};

struct server {
	struct user connected_users[SERVER_MAX_CONNECTED_USERS];
	int num_connected_users;
	int max_connected_users;
	int server_socket_fd;
	struct sockaddr_in server_addr_info;
	struct sockaddr_in client_addr_info;
	int server_addr_info_length;
	int client_addr_info_length;
};

int create_server(struct server * server);
void * connection_handler(void * server);
int start_server(struct server * server);
int send_response_to_client(struct user * user, uint8_t response);
int create_user(struct user * user, struct sockaddr_in user_addr_info, char nickname[SERVER_NICKNAME_LEN]);
int add_new_user(struct server * server, struct user * user);
int remove_user(struct server * server, struct user * user);
void list_users(struct server * server);
void print_user_info(struct user * user);
int process_packet(struct server * server, struct rtlp_packet * rtlp_packet_in);
struct user * search_user(struct server * server, char nickname[SERVER_NICKNAME_LEN]);
int ack(struct server * server, struct rtlp_packet * rtlp_packet_in, uint8_t response, uint8_t data[RTLP_DATA_LEN]);

#endif
