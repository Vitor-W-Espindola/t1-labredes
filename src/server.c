#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <pthread.h>
#include <time.h>
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
	struct server * s = (struct server *) server; // Server struct
	
	struct sockaddr_in client_si;
	int client_socket_fd;
	int client_addr_len = sizeof(client_si);
	client_socket_fd = accept(s->server_socket_fd, (struct sockaddr *) &client_si, &client_addr_len);

	printf("Client connected %s:%d\n", inet_ntoa(client_si.sin_addr), ntohs(client_si.sin_port));

	struct user u;
	memset(&u, 0, sizeof(u));
	char new_nickname[SERVER_NICKNAME_LEN];
	sprintf(new_nickname, "%s:%d", inet_ntoa(client_si.sin_addr), ntohs(client_si.sin_port));
	u.allow_transfer = 0;
	create_user(&u, client_socket_fd, client_si, new_nickname);
	add_new_user(s, &u);
		
	struct rtlp_packet rtlp_packet;
	uint8_t operation = RTLP_OPERATION_SERVER_MSG;
	uint8_t source[RTLP_SOURCE_LEN];
	uint8_t destination[RTLP_DESTINATION_LEN];
	uint8_t data[RTLP_DATA_LEN];
	uint8_t packet_buf[SERVER_BUF_LEN];

	s->num_connected_users += 1;	
	if(s->num_connected_users > s->max_connected_users) {

		memset(source, 0, RTLP_SOURCE_LEN);
		memset(destination, 0, RTLP_DESTINATION_LEN);
		memset(data, 0, RTLP_DATA_LEN);
		rtlp_packet_build(&rtlp_packet, operation, source, destination, data, RTLP_TYPE_SERVER_TO_CLIENT_ACK, RTLP_RESPONSE_FULL_SERVER, RTLP_TRANSPORT_PROTOCOL_TCP);
		
		memset(packet_buf, 0, SERVER_BUF_LEN);		
		memcpy(packet_buf, &rtlp_packet, SERVER_BUF_LEN);
		write(client_socket_fd, packet_buf, SERVER_BUF_LEN);
		
		printf("OUT:\n");
		print_rtlp_packet(&rtlp_packet);
		
		printf("Closed connection with %s:%d (Server Full)\n", inet_ntoa(client_si.sin_addr), ntohs(client_si.sin_port));
		
		remove_user(s, &u);
		s->num_connected_users -= 1;
		
		close(client_socket_fd);	
		
		return NULL;
	} else {
		// Sends an ACK OK with the the clients nickname
		memset(source, 0, RTLP_SOURCE_LEN);
		memset(destination, 0, RTLP_DESTINATION_LEN);
		memset(data, 0, RTLP_DATA_LEN);
		strcpy(destination, new_nickname);
		strcpy(data, new_nickname); 	
		rtlp_packet_build(&rtlp_packet, operation, source, destination, data, RTLP_TYPE_SERVER_TO_CLIENT_ACK, RTLP_RESPONSE_OK, RTLP_TRANSPORT_PROTOCOL_TCP);
		memset(packet_buf, 0, SERVER_BUF_LEN);		
		memcpy(packet_buf, &rtlp_packet, SERVER_BUF_LEN);
		write(client_socket_fd, packet_buf, SERVER_BUF_LEN); 
		
		printf("OUT:\n");
		print_rtlp_packet(&rtlp_packet);

		// Sends an welcome message
		char welcome_message[RTLP_DATA_LEN];
		time_t mytime = time(NULL);
		char * time_str = ctime(&mytime);
		time_str[strlen(time_str) - 1] = 0x20;	
		snprintf(welcome_message, RTLP_DATA_LEN, "Welcome.\nYour nickname %s.\nLocaltime: %s\nOnline users: %d", new_nickname, time_str, s->num_connected_users);	
		
		memset(source, 0, RTLP_SOURCE_LEN);
		memset(destination, 0, RTLP_DESTINATION_LEN);
		memset(data, 0, RTLP_DATA_LEN);
		strcpy(destination, new_nickname);
		strcpy(data, welcome_message);
		rtlp_packet_build(&rtlp_packet, operation, source, destination, data, RTLP_TYPE_SERVER_TO_CLIENT_WELCOME, RTLP_RESPONSE_OK, RTLP_TRANSPORT_PROTOCOL_TCP);
		memset(packet_buf, 0, SERVER_BUF_LEN);		
		memcpy(packet_buf, &rtlp_packet, SERVER_BUF_LEN);
		write(client_socket_fd, packet_buf, SERVER_BUF_LEN);
 
		printf("OUT:\n");
		print_rtlp_packet(&rtlp_packet);

		// Starts receiving commands from user	
		struct rtlp_packet rtlp_packet_in;
		uint8_t buf_in[SERVER_BUF_LEN];
		memset(&buf_in, 0, SERVER_BUF_LEN);
		memset(&packet_buf, 0, SERVER_BUF_LEN);
		int recv_len = 0;
		while(1) {
			// Receives data from client, blocking the thread
			recv_len = read(client_socket_fd, buf_in, SERVER_BUF_LEN);
			if (recv_len == -1 || recv_len == 0) break;

			// Casts bytes to a rtlp_packet
			memcpy(&rtlp_packet_in, buf_in, SERVER_BUF_LEN);

			// Process packet
			process_packet(s, &rtlp_packet_in);

			for(int i = 0; i < SERVER_BUF_LEN; i++)
				printf("%x ", buf_in[i]);
			printf("\n");
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

int send_response_to_client(struct user * user, uint8_t response) {

	// Sends an response of type RTLP_RESPONSE_response to client
	struct rtlp_packet rtlp_packet;
	uint8_t packet_buf[SERVER_BUF_LEN];

	uint8_t source[RTLP_SOURCE_LEN], destination[RTLP_DESTINATION_LEN], data[RTLP_DATA_LEN];
	memset(source, 0, RTLP_SOURCE_LEN);
	memset(destination, 0, RTLP_DESTINATION_LEN);
	memset(data, 0, RTLP_DATA_LEN);
	memset(&rtlp_packet, 0, SERVER_BUF_LEN);	

	strcpy(destination, user->nickname);
	
	rtlp_packet_build(&rtlp_packet, RTLP_OPERATION_SERVER_MSG, source, destination, data, RTLP_TYPE_SERVER_TO_CLIENT_ACK, response, RTLP_TRANSPORT_PROTOCOL_TCP);

	printf("\n");
	printf("OUT:");
	print_rtlp_packet(&rtlp_packet);
	printf("\n");
	
	memset(packet_buf, 0, SERVER_BUF_LEN);		
	memcpy(packet_buf, &rtlp_packet, SERVER_BUF_LEN);

	return write(user->user_socket_fd, packet_buf, SERVER_BUF_LEN);
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
	printf("allow_transfer :%d\n", user->allow_transfer);	
}

// Function used by thread for processing the packet
// and modify server structure
int process_packet(struct server * server, struct rtlp_packet * rtlp_packet_in) {
	uint8_t source[RTLP_SOURCE_LEN], destination[RTLP_DESTINATION_LEN], data[RTLP_DATA_LEN], packet_buf[SERVER_BUF_LEN];
	int user_found = 0;
	struct user * u;
	struct rtlp_packet rtlp_packet_out;
	char nickname_aux[SERVER_NICKNAME_LEN];
	
	memset(source, 0, RTLP_SOURCE_LEN);
	memset(destination, 0, RTLP_DESTINATION_LEN);
	memset(data, 0, RTLP_DATA_LEN);
	memset(packet_buf, 0, SERVER_BUF_LEN);
	memset(&rtlp_packet_out, 0, sizeof(struct rtlp_packet));
	memset(nickname_aux, 0, SERVER_NICKNAME_LEN);	

	switch(rtlp_packet_in->operation) {
		case RTLP_OPERATION_CLIENT_SENDALL:
			// source -> nickname
			// destination -> empty
			// data -> message
			printf("\n");
			printf("Processing RTLP_OPERATION_CLIENT_SENDALL operation...\n");
			print_rtlp_packet(rtlp_packet_in);
			printf("\n");
			
			rtlp_packet_build(&rtlp_packet_out, RTLP_OPERATION_SERVER_MSG, rtlp_packet_in->source, destination, rtlp_packet_in->data, RTLP_TYPE_SERVER_TO_CLIENT_PB_ASYNC, RTLP_RESPONSE_NONE, RTLP_TRANSPORT_PROTOCOL_TCP);
			
			memset(packet_buf, 0, SERVER_BUF_LEN);		
			memcpy(packet_buf, &rtlp_packet_out, SERVER_BUF_LEN);

			for(int i = 0; i < server->num_connected_users; i++)
				write(server->connected_users[i].user_socket_fd, packet_buf, SERVER_BUF_LEN); 
			
			ack(server, rtlp_packet_in, RTLP_RESPONSE_OK);	
			
			break;

		case RTLP_OPERATION_CLIENT_SENDPV:	
			// source -> source nickname
			// destination -> destination nickname
			// data -> message
			
			printf("\n");
			printf("Processing RTLP_OPERATION_CLIENT_SENDPV operation...\n");
			print_rtlp_packet(rtlp_packet_in);
			printf("\n");

			memcpy(nickname_aux, rtlp_packet_in->destination, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {	
				rtlp_packet_build(&rtlp_packet_out, RTLP_OPERATION_SERVER_MSG, rtlp_packet_in->source, rtlp_packet_in->destination, rtlp_packet_in->data, RTLP_TYPE_SERVER_TO_CLIENT_PV_ASYNC, RTLP_RESPONSE_NONE, RTLP_TRANSPORT_PROTOCOL_TCP);

				memset(packet_buf, 0, SERVER_BUF_LEN);		
				memcpy(packet_buf, &rtlp_packet_out, SERVER_BUF_LEN);

				printf("Sending private message to file descriptor: %d...\n", u->user_socket_fd);
				write(u->user_socket_fd, packet_buf, SERVER_BUF_LEN);

				ack(server, rtlp_packet_in, RTLP_RESPONSE_OK);	

				break;
			} else 	ack(server, rtlp_packet_in, RTLP_RESPONSE_USER_NOT_FOUND);	

			break;
		case RTLP_OPERATION_CLIENT_TRANSFERPV:
			printf("\n");
			printf("Processing RTLP_OPERATION_CLIENT_TRANSFERPV operation...");
			print_rtlp_packet(rtlp_packet_in);
			printf("\n");

			memcpy(nickname_aux, rtlp_packet_in->destination, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {
				if(u->allow_transfer != 0) {
					rtlp_packet_build(&rtlp_packet_out, RTLP_OPERATION_SERVER_MSG, rtlp_packet_in->source, rtlp_packet_in->destination, rtlp_packet_in->data, RTLP_TYPE_SERVER_TO_CLIENT_CHUNK, RTLP_RESPONSE_NONE, RTLP_TRANSPORT_PROTOCOL_TCP);
					memset(packet_buf, 0, SERVER_BUF_LEN);		
					memcpy(packet_buf, &rtlp_packet_out, SERVER_BUF_LEN);

					printf("Transfering file to file descriptor: %d...\n", u->user_socket_fd);
					write(u->user_socket_fd, packet_buf, SERVER_BUF_LEN);
					
					ack(server, rtlp_packet_in, RTLP_RESPONSE_OK);
				} else ack(server, rtlp_packet_in, RTLP_RESPONSE_USER_NOT_AVAILABLE);	
			} else 	ack(server, rtlp_packet_in, RTLP_RESPONSE_USER_NOT_FOUND);	

			break;
		case RTLP_OPERATION_CLIENT_ENABLETRANSFER:
			
			memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {
				print_user_info(u);
				u->allow_transfer = 1;	
				ack(server, rtlp_packet_in, RTLP_RESPONSE_OK);	
			}
			
			break;

		case RTLP_OPERATION_CLIENT_DISABLETRANSFER:
			memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {
				print_user_info(u);
				u->allow_transfer = 0;	
				ack(server, rtlp_packet_in, RTLP_RESPONSE_OK);	
			}

			break;
		default:
			break;
	}
	return 0;
}

struct user * search_user(struct server * server, char nickname[SERVER_NICKNAME_LEN]) {	
	// Searches for the destination
	for(int i = 0; i < server->num_connected_users; i++) {
		if(!strcmp(server->connected_users[i].nickname, nickname))
			return &(server->connected_users[i]);
	}
	return NULL;
}

int ack(struct server * server, struct rtlp_packet * rtlp_packet_in, int8_t response) {
	struct user * u;
	struct rtlp_packet rtlp_packet_out;
	char nickname_aux[SERVER_NICKNAME_LEN];
	uint8_t packet_buf[SERVER_BUF_LEN];
	
	memset(&rtlp_packet_out, 0, sizeof(struct rtlp_packet));
	memset(nickname_aux, 0, SERVER_NICKNAME_LEN);
	memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
	u = search_user(server, nickname_aux);
	rtlp_packet_build(&rtlp_packet_out, RTLP_OPERATION_SERVER_MSG, rtlp_packet_in->source, rtlp_packet_in->destination, rtlp_packet_in->data, RTLP_TYPE_SERVER_TO_CLIENT_ACK, response, RTLP_TRANSPORT_PROTOCOL_TCP);
	memset(packet_buf, 0, SERVER_BUF_LEN);		
	memcpy(packet_buf, &rtlp_packet_out, SERVER_BUF_LEN);

	int w_len = write(u->user_socket_fd, packet_buf, SERVER_BUF_LEN);

	printf("OUT:\n");
	print_rtlp_packet(&rtlp_packet_out);

	return w_len;	
}
