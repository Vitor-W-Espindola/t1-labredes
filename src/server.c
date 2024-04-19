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
	// connection-less based (SOCK_DGRAM), 
	// especifically UDP (IPPROTO_TCP).
	int server_socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int client_socket_fd;
	if(server_socket_fd == -1) return server_socket_fd;	

	// Binding configuration	
	struct sockaddr_in server_si, client_si; // Stores address and port info from server
	int server_si_length = sizeof(server_si);
	int client_si_length = sizeof(client_si);
	memset((char *) &server_si, 0, server_si_length);
	memset((char *) &client_si, 0, client_si_length);	

	// Binding configuration
	server_si.sin_family = AF_INET;
	server_si.sin_port = htons(SERVER_PORT);
	server_si.sin_addr.s_addr = htonl(INADDR_ANY);

	// Names the socket by binding it
	int bind_res = bind(server_socket_fd, (struct sockaddr *) &server_si, sizeof(server_si));
	if(bind_res == -1) return bind_res;
	
	// Populates the server structure
	server->num_connected_users = 0;
	server->max_connected_users = SERVER_MAX_CONNECTED_USERS;
	// Empty connected users buffer
	for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++) {
		struct user connected_user = { 0 };
		server->connected_users[i] = connected_user; 
	}
	server->server_socket_fd = server_socket_fd;
	server->server_addr_info = server_si;
	server->server_addr_info_length = server_si_length;
	server->client_addr_info = client_si;
	server->client_addr_info_length = client_si_length;
	return 0;
}

int start_server(struct server * server) {
	struct rtlp_packet rtlp_packet;
	uint8_t packet_buf[SERVER_BUF_LEN];
	uint8_t operation = RTLP_OPERATION_SERVER_MSG;
	uint8_t source[RTLP_SOURCE_LEN];
	uint8_t destination[RTLP_DESTINATION_LEN];
	uint8_t data[RTLP_DATA_LEN];
	while(1) {

		memset(source, 0, RTLP_SOURCE_LEN);
		memset(destination, 0, RTLP_DESTINATION_LEN);
		memset(data, 0, RTLP_DATA_LEN);
		memset(packet_buf, 0, SERVER_BUF_LEN);

		// Receives data
		int recv_len = recvfrom(server->server_socket_fd, packet_buf, SERVER_BUF_LEN, 0, (struct sockaddr *) &server->client_addr_info, &server->client_addr_info_length);

		memcpy(&rtlp_packet, packet_buf, SERVER_BUF_LEN);		

		// Process packet
		process_packet(server, &rtlp_packet);

	}
}

int create_user(struct user * user, struct sockaddr_in user_addr_info, char nickname[SERVER_NICKNAME_LEN]) {
	user->user_addr_info = user_addr_info;
	strcpy(user->nickname, nickname);
	return 0;
}

int add_new_user(struct server * server, struct user * user) {
	for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++) {
		if(!strcmp(server->connected_users[i].nickname, "")) {
			memcpy(&(server->connected_users[i]), user, sizeof(struct user));	
			break;
		}
	}	
	return 0;
}

int remove_user(struct server * server, struct user * user) {
	for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++) {
		printf("Comparing %s with %s\n", server->connected_users[i].nickname, user->nickname);
		if(!strcmp(server->connected_users[i].nickname, user->nickname)) {
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
	printf("user_addr_info:%s:%d\n", inet_ntoa(user->user_addr_info.sin_addr), ntohs(user->user_addr_info.sin_port));
	printf("nickname: %s\n", user->nickname);
	printf("allow_transfer :%d\n", user->allow_transfer);	
}

// Function used by thread for processing the packet
// and modify server structure
int process_packet(struct server * server, struct rtlp_packet * rtlp_packet_in) {

	uint8_t source[RTLP_SOURCE_LEN], destination[RTLP_DESTINATION_LEN], data[RTLP_DATA_LEN], packet_buf[SERVER_BUF_LEN];
	struct user * u;
	struct rtlp_packet rtlp_packet_out;
	char nickname_aux[SERVER_NICKNAME_LEN];
	int write_len = 0;	

	memset(source, 0, RTLP_SOURCE_LEN);
	memset(destination, 0, RTLP_DESTINATION_LEN);
	memset(data, 0, RTLP_DATA_LEN);
	memset(packet_buf, 0, SERVER_BUF_LEN);
	memset(&rtlp_packet_out, 0, sizeof(struct rtlp_packet));
	memset(nickname_aux, 0, SERVER_NICKNAME_LEN);	

	switch(rtlp_packet_in->operation) {
		case RTLP_OPERATION_CLIENT_REQNICKNAME:

			struct user new_user = { 0 };
			memset(&new_user, 0, sizeof(u));
			char new_nickname[SERVER_NICKNAME_LEN] = { 0 };
			sprintf(new_nickname, "%s:%d", inet_ntoa(server->client_addr_info.sin_addr), ntohs(server->client_addr_info.sin_port));
			new_user.allow_transfer = 0;
			new_user.user_addr_info = server->client_addr_info;
			new_user.user_addr_info_length = server->client_addr_info_length;
			create_user(&new_user, server->client_addr_info, new_nickname);
			add_new_user(server, &new_user);
				
			server->num_connected_users += 1;	
			if(server->num_connected_users > server->max_connected_users) {
				// Sends FULL SERVER response to client
				rtlp_packet_build(&rtlp_packet_out, RTLP_OPERATION_SERVER_MSG, source, destination, data, RTLP_TYPE_SERVER_TO_CLIENT_ACK, RTLP_RESPONSE_FULL_SERVER, RTLP_TRANSPORT_PROTOCOL_TCP);	
				memcpy(packet_buf, &rtlp_packet_out, SERVER_BUF_LEN);
				
				write_len = sendto(server->server_socket_fd, packet_buf, SERVER_BUF_LEN, 0, (struct sockaddr *) &server->client_addr_info, server->client_addr_info_length);
				
				printf("OUT:\n");
				print_rtlp_packet(&rtlp_packet_out);
				
				remove_user(server, &new_user);
				server->num_connected_users -= 1;
					
			} else {
				// Sends an ACK OK with the the clients nickname
				strcpy(destination, new_nickname);
				strcpy(data, new_nickname); 	
				rtlp_packet_build(&rtlp_packet_out, RTLP_OPERATION_SERVER_MSG, source, destination, data, RTLP_TYPE_SERVER_TO_CLIENT_ACK, RTLP_RESPONSE_OK, RTLP_TRANSPORT_PROTOCOL_TCP);
				memset(packet_buf, 0, SERVER_BUF_LEN);		
				memcpy(packet_buf, &rtlp_packet_out, SERVER_BUF_LEN);
				
				write_len = sendto(server->server_socket_fd, packet_buf, SERVER_BUF_LEN, 0, (struct sockaddr *) &server->client_addr_info, server->client_addr_info_length);
				
				printf("OUT:\n");
				print_rtlp_packet(&rtlp_packet_out);

				// Sends an welcome message
				char welcome_message[RTLP_DATA_LEN];
				time_t mytime = time(NULL);
				char * time_str = ctime(&mytime);
				time_str[strlen(time_str) - 1] = 0x20;	
				snprintf(welcome_message, RTLP_DATA_LEN, "Welcome.\nYour nickname %s.\nLocaltime: %s\nOnline users: %d", new_nickname, time_str, server->num_connected_users);	
				
				memset(destination, 0, RTLP_DESTINATION_LEN);
				memset(data, 0, RTLP_DATA_LEN);
				strcpy(destination, new_nickname);
				strcpy(data, welcome_message);
				rtlp_packet_build(&rtlp_packet_out, RTLP_OPERATION_SERVER_MSG, source, destination, data, RTLP_TYPE_SERVER_TO_CLIENT_WELCOME, RTLP_RESPONSE_OK, RTLP_TRANSPORT_PROTOCOL_TCP);
				memset(packet_buf, 0, SERVER_BUF_LEN);		
				memcpy(packet_buf, &rtlp_packet_out, SERVER_BUF_LEN);
				write_len = sendto(server->server_socket_fd, packet_buf, SERVER_BUF_LEN, 0, (struct sockaddr *) &server->client_addr_info, server->client_addr_info_length);
				
				printf("OUT:\n");
				print_rtlp_packet(&rtlp_packet_out);
			}			
			break;
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
				write_len = sendto(server->server_socket_fd, packet_buf, SERVER_BUF_LEN, 0, (struct sockaddr *) &server->connected_users[i].user_addr_info, server->connected_users[i].user_addr_info_length);
			
			ack(server, rtlp_packet_in, RTLP_RESPONSE_OK, data);	
			
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

				printf("Sending private message to user: %s...\n", u->nickname);
				write_len = sendto(server->server_socket_fd, packet_buf, SERVER_BUF_LEN, 0, (struct sockaddr *) &u->user_addr_info, u->user_addr_info_length);
				ack(server, rtlp_packet_in, RTLP_RESPONSE_OK, data);	

				break;
			} else 	ack(server, rtlp_packet_in, RTLP_RESPONSE_USER_NOT_FOUND, data);	

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

					printf("Transfering file to user: %s...\n", u->nickname);
					write_len = sendto(server->server_socket_fd, packet_buf, SERVER_BUF_LEN, 0, (struct sockaddr *) &u->user_addr_info, u->user_addr_info_length);
					
					ack(server, rtlp_packet_in, RTLP_RESPONSE_OK, data);
				} else ack(server, rtlp_packet_in, RTLP_RESPONSE_USER_NOT_AVAILABLE, data);	
			} else ack(server, rtlp_packet_in, RTLP_RESPONSE_USER_NOT_FOUND, data);	

			break;
		case RTLP_OPERATION_CLIENT_ENABLETRANSFER:
			
			memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {
				print_user_info(u);
				u->allow_transfer = 1;	
				ack(server, rtlp_packet_in, RTLP_RESPONSE_TRANSFERENABLED, data);	
			}
			
			break;

		case RTLP_OPERATION_CLIENT_DISABLETRANSFER:
			memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {
				print_user_info(u);
				u->allow_transfer = 0;	
				ack(server, rtlp_packet_in, RTLP_RESPONSE_TRANSFERDISABLED, data);	
			}

			break;
		
		case RTLP_OPERATION_CLIENT_NICKNAME:
		
			printf("\n");
			printf("Processing RTLP_OPERATION_CLIENT_NICKNAME operation...");
			print_rtlp_packet(rtlp_packet_in);
			printf("\n");
			
			memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {
				memcpy(nickname_aux, rtlp_packet_in->data, SERVER_NICKNAME_LEN);
				if(strlen(rtlp_packet_in->data) <= SERVER_NICKNAME_LEN) {
					memset(u->nickname, 0, SERVER_NICKNAME_LEN);
					memset(rtlp_packet_in->source, 0, RTLP_SOURCE_LEN);
					memset(rtlp_packet_in->data, 0, RTLP_DATA_LEN);
					strncpy(u->nickname, nickname_aux, SERVER_NICKNAME_LEN);
					strncpy(rtlp_packet_in->source, u->nickname, SERVER_NICKNAME_LEN);
					strncpy(data, u->nickname, SERVER_NICKNAME_LEN);
					list_users(server);
					
					ack(server, rtlp_packet_in, RTLP_RESPONSE_NEW_NICKNAME, data);
				} else ack(server, rtlp_packet_in, RTLP_RESPONSE_NICKNAME_TOO_LONG, data);
			}
			break;
		case RTLP_OPERATION_CLIENT_LISTUSERS:
			printf("\n");
			printf("Processing RTLP_OPERATION_CLIENT_LISTUSERS operation...");
			print_rtlp_packet(rtlp_packet_in);
			printf("\n");

			memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {
				char users_buf[RTLP_DATA_LEN] = { 0 };
				strcpy(users_buf, "Online users:");
				int users_buf_index = strlen(users_buf);
				users_buf[users_buf_index++] = '\n';
				for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++) {	
					strcpy(&users_buf[users_buf_index], server->connected_users[i].nickname);
					users_buf_index = strlen(users_buf);
					users_buf[users_buf_index++] = '\n';
				}
				users_buf[users_buf_index++] = '\0';
				printf("users_buf: %s", users_buf);
				strcpy(data, users_buf);

				ack(server, rtlp_packet_in, RTLP_RESPONSE_LISTUSERS, data);
			} else { printf("\n\nUser is null\n\n"); }
			break;
		case RTLP_OPERATION_CLIENT_QUIT:
			printf("\n");
			printf("Processing RTLP_OPERATION_CLIENT_QUIT operation...");
			print_rtlp_packet(rtlp_packet_in);
			printf("\n");

			memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
			u = search_user(server, nickname_aux);
			if(u != NULL) {
				remove_user(server, u);	
				server->num_connected_users -= 1;	
			}
			
			break;
		default:
			break;
	}
	return 0;
}

struct user * search_user(struct server * server, char nickname[SERVER_NICKNAME_LEN]) {	
	// Searches for the destination
	for(int i = 0; i < SERVER_MAX_CONNECTED_USERS; i++) {
		if(!strcmp(server->connected_users[i].nickname, nickname))
			return &(server->connected_users[i]);
	}
	return NULL;
}

int ack(struct server * server, struct rtlp_packet * rtlp_packet_in, uint8_t response, uint8_t data[RTLP_DATA_LEN]) {
	int write_len = 0;
	struct user * u;
	struct rtlp_packet rtlp_packet_out;
	char nickname_aux[SERVER_NICKNAME_LEN];
	uint8_t packet_buf[SERVER_BUF_LEN];
	uint8_t source[RTLP_SOURCE_LEN];
	memset(source, 0, RTLP_SOURCE_LEN);
	memset(&rtlp_packet_out, 0, sizeof(struct rtlp_packet));
	
	rtlp_packet_build(&rtlp_packet_out, RTLP_OPERATION_SERVER_MSG, source, rtlp_packet_in->source, data, RTLP_TYPE_SERVER_TO_CLIENT_ACK, response, RTLP_TRANSPORT_PROTOCOL_TCP);
	
	memset(packet_buf, 0, SERVER_BUF_LEN);		
	memcpy(packet_buf, &rtlp_packet_out, SERVER_BUF_LEN);
	
	memset(nickname_aux, 0, SERVER_NICKNAME_LEN);
	memcpy(nickname_aux, rtlp_packet_in->source, SERVER_NICKNAME_LEN);
	
	u = search_user(server, nickname_aux);
	if(u == NULL) {
		printf("u is NULL\n");
		return 0;
	}	
	
	write_len = sendto(server->server_socket_fd, packet_buf, SERVER_BUF_LEN, 0, (struct sockaddr *) &u->user_addr_info, u->user_addr_info_length);
	
	printf("OUT:\n");
	print_rtlp_packet(&rtlp_packet_out);

	return write_len;	
}
