#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>
#include "server.h"
#include "rtlp.h"

int create_server(struct server * server) {

	// Creates a file descriptor for a socket which works
	// with domain of IPV4 internet addresses (AF_INET),
	// connection-based byte streams (SOCK_STREAM), 
	// especifically TCP (IPPROTO_TCP).
	int server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(server_socket_fd == -1) return server_socket_fd;	

	// Binding configuration	
	struct sockaddr_in server_si; // Stores address and port info from server
	memset((char *) &server_si, 0, sizeof(server_si));
	
	// Binding configuration
	server_si.sin_family = AF_INET;
	server_si.sin_port = htons(SERVER_PORT);
	server_si.sin_addr.s_addr = htonl(INADDR_ANY);

	// Names the socket by binding it
	int bind_res = bind(server_socket_fd, (struct sockaddr *) &server_si, sizeof(server_si));
	if(bind_res == -1) return bind_res;

	// Makes a passive socket that only queue up SERVER_MAX_PENDING_CONNECTIONS requests
	int listen_res = listen(server_socket_fd, SERVER_MAX_PENDING_CONNECTIONS);
	if(listen_res == -1) return listen_res; 

	// Populates the server structure
	server->num_connected_users = 0;
	server->max_connected_users = SERVER_MAX_CONNECTED_USERS;
	// Empty connected users buffer
	for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++) {
		struct user connected_user;
		memset(&connected_user, 0, sizeof(struct user));
		server->connected_users[i] = connected_user; 
	}
	server->server_socket_fd = server_socket_fd;
	server->server_addr_info = server_si;
	return 0;
}

void * connection_handler(void * server) {
	// This is the handler which takes care of the user during all his stay on the server
	struct server * s = (struct server *) server; // Server struct
	
	// A File Descriptor conn for the new connected socket
	struct sockaddr_in client_si;
	int client_socket_fd;
	int client_addr_len = sizeof(client_si);
	client_socket_fd = accept(s->server_socket_fd, (struct sockaddr *) &client_si, &client_addr_len);

	printf("Client connected %s:%d\n", inet_ntoa(client_si.sin_addr), ntohs(client_si.sin_port));
	
	struct user u;
	memset(&u, 0, sizeof(u));
	create_user(&u, client_socket_fd, client_si, inet_ntoa(client_si.sin_addr));
	struct rtlp_packet rtlp_packet;
	uint8_t packet_buf[SERVER_BUF_LEN];
	if(s->num_connected_users >= s->max_connected_users) {

		// Sends an ACK FULL_SERVER and closes connection if server is full	
		send_response_to_client(client_socket_fd, RTLP_RESPONSE_FULL_SERVER);
	
		printf("Closed connection with %s:%d (Server Full)\n", inet_ntoa(client_si.sin_addr), ntohs(client_si.sin_port));
		
		close(client_socket_fd);	
		return NULL;
	} else {
		add_new_user(s, &u);

		s->num_connected_users += 1;	

		// Sends an ACK OK		
		send_response_to_client(client_socket_fd, RTLP_RESPONSE_OK);
	
		// Starts receiving commands from user	
		char buf[SERVER_BUF_LEN];
		memset(buf, 0, sizeof(buf));
		int recv_len = 0;
		while(1) {
			// Receives data from client, blocking the thread
			recv_len = read(client_socket_fd, buf, SERVER_BUF_LEN);
			if (recv_len == -1 || recv_len == 0) break;

			// Casts bytes to a rtlp_packet
			memcpy(&rtlp_packet, packet_buf, SERVER_BUF_LEN);
			printf("Data received\n");

			for(int i = 0; i < SERVER_BUF_LEN; i++)
				printf("%x ", buf[i]);
			printf("\n");

			// Sends an ACK OK to client		
			send_response_to_client(client_socket_fd, RTLP_RESPONSE_OK);
		}
		remove_user(s, &u);
		s->num_connected_users -= 1;
		printf("Closed connenction with %s:%d\n", inet_ntoa(client_si.sin_addr), ntohs(client_si.sin_port));
		close(client_socket_fd);
	}	
}

int start_server(struct server * server) {
	struct pollfd server_socket_poll;
	memset(&server_socket_poll, 0, sizeof(server_socket_poll));
	server_socket_poll.fd = server->server_socket_fd;
	server_socket_poll.events = POLLIN;
	
	while(poll(&server_socket_poll, 1, -1) != -1) {
		if(server_socket_poll.revents & POLLIN) {
			pthread_t server_thread_id;
			pthread_create(&server_thread_id, NULL, connection_handler, server);
		}
	}
}

int send_response_to_client(int client_socket_fd, uint8_t response) {

	// Sends an response of type RTLP_RESPONSE_response to client
	struct rtlp_packet rtlp_packet;
	uint8_t packet_buf[SERVER_BUF_LEN];

	uint8_t operation_first_parameter[RTLP_OPERATION_FIRST_PARAMETER_LEN], operation_second_parameter[RTLP_OPERATION_SECOND_PARAMETER_LEN];
	memset(operation_first_parameter, 0, RTLP_OPERATION_FIRST_PARAMETER_LEN);
	memset(operation_second_parameter, 0, RTLP_OPERATION_SECOND_PARAMETER_LEN);

	memset(&rtlp_packet, 0, SERVER_BUF_LEN);	

	rtlp_packet_build(&rtlp_packet, RTLP_OPERATION_SERVER_MSG, operation_first_parameter, operation_second_parameter, RTLP_TYPE_SERVER_TO_CLIENT_ACK, response, RTLP_TRANSPORT_PROTOCOL_TCP);

	memset(packet_buf, 0, SERVER_BUF_LEN);		

	memcpy(packet_buf, &rtlp_packet, SERVER_BUF_LEN);

	return write(client_socket_fd, packet_buf, SERVER_BUF_LEN);
}

int create_user(struct user * user, int user_socket_fd, struct sockaddr_in user_addr_info, char nickname[SERVER_NICKNAME_LEN]) {
	user->user_socket_fd = user_socket_fd;
	user->user_addr_info = user_addr_info;
	strcpy(user->nickname, nickname);
	return 0;
}

int add_new_user(struct server * server, struct user * user) {
	for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++) {
		if(server->connected_users[i].user_socket_fd == 0) {
			memcpy(&(server->connected_users[i]), user, sizeof(struct user));	
			break;
		}
	}	
	return 0;
}

int remove_user(struct server * server, struct user * user) {
	for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++) {
		if(server->connected_users[i].user_socket_fd == user->user_socket_fd) {
			memset(&(server->connected_users[i]), 0, sizeof(struct user));
			break;
		}
	}
}

void list_users(struct server * server) {
	printf("Lising users...\n");
	for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++)
		print_user_info(&(server->connected_users[i]));
}

void print_user_info(struct user * user) {
	printf("User info:\n");
	printf("user_socket_fd: %d\n", user->user_socket_fd);
	printf("user_addr_info:%s:%d\n", inet_ntoa(user->user_addr_info.sin_addr), ntohs(user->user_addr_info.sin_port));
	printf("nickname: %s\n", user->nickname);
}
