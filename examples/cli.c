#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../src/client.h"
#include "../src/server.h"

void die(char *s) {
	perror(s);
	exit(1);
}

int main(int argc, char **argv) {

	/*
	printf("Trying to connect...\n");
	printf("\n");
	printf("Welcome to the Ride the Lightning Chat Server\n");
	printf("Your nickname is currently 192.168.0.2\n");
	printf("Here is a list of commands for you to use:\n");
	printf("\t- sendall <message>: sends a broadcast message for all users connected\n");
	printf("\t- sendpv <nickname> <message>: sends a private message for user nickname\n");
	printf("\t- transferpv <nickname> <file>: sends a transfer file request to user nickname to transfer the file file.\n");
	printf("\t- transferpvaccept <nickname>: accepts the transfer file request from user nickname, if there is any.\n");
	printf("\t- transferpvdecline <nickname>: decliens the transfer file rqeuest from user nickname, if there is any.\n");
	printf("\t- nickname <new_nickname>: sets a new nickname\n");
	printf("\t- list: lists all online users\n");
	printf("\t- quit: quits the server\n");
	printf("\n");
	printf("For any further help, please contact one of our administrators online at the moment...\n");
	printf("\n");
	printf("\t\t\t\t\tEnjoy =)\n");
	printf("\n");
	printf("Command: ");
	*/	
	
	if(argc != 3) {
		printf("Usage: %s <hostname> <port>\n", argv[0]);
		exit(0);
	}
		
	// Stores server address info
	struct sockaddr_in server_addr;
	memset((char *) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_aton(argv[1], &server_addr.sin_addr);
	server_addr.sin_port = htons(atoi(argv[2]));

	// Creates socket File Descriptor
	int socket_fd;
	socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(socket_fd < 0) die("socket");		

	// Create a connection with the server
	int connection = connect(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));
	if(connection < 0) die("connect");

	// Receives an ACK from server
	struct rtlp_packet rtlp_packet;
	uint8_t packet_buf[SERVER_BUF_LEN];
	memset(packet_buf, 0, SERVER_BUF_LEN);
	
	int recv_len = read(socket_fd, packet_buf, SERVER_BUF_LEN);
	if (recv_len == -1) die("read"); 

	// Casts bytes to a rtlp_packet
	memset(&rtlp_packet, 0, sizeof(rtlp_packet));
	memcpy(&rtlp_packet, packet_buf, SERVER_BUF_LEN);
	
	if(rtlp_packet.response == RTLP_RESPONSE_FULL_SERVER) printf("The server is full.\n");
	else {
		while(1) {
			// Reads command from cli
			uint8_t cmd_buf[CLIENT_CMD_LEN];
			memset(cmd_buf, 0, CLIENT_CMD_LEN);
			
			printf("Command: ");
		
			fgets(cmd_buf, CLIENT_CMD_LEN, stdin);
			cmd_buf[strcspn(cmd_buf, "\r\n")] = ' ';
			cmd_buf[CLIENT_CMD_LEN - 1] = '\0';
			
			// Translate command to rtlp_packet
			memset(&rtlp_packet, 0, sizeof(rtlp_packet));
			if(from_command_to_packet(cmd_buf, &rtlp_packet) == 1) {
				printf("Invalid command.\n");
				break;;
			};
				
			// Sends rtlp packet through socket
			memset(packet_buf, 0, SERVER_BUF_LEN);
			memcpy(packet_buf, &rtlp_packet, sizeof(rtlp_packet));
			
			int write_len = write(socket_fd, packet_buf, SERVER_BUF_LEN);
			if (write_len < 0) die("write");
			
			print_rtlp_packet(&rtlp_packet);

			if(rtlp_packet.operation == RTLP_OPERATION_CLIENT_QUIT) break;	
			// Receives an ACK from server
			memset(packet_buf, 0, SERVER_BUF_LEN);
			
			int recv_len = read(socket_fd, packet_buf, SERVER_BUF_LEN);
			if (recv_len == -1) die("read"); 

			// Casts bytes to a rtlp_packet
			memset(&rtlp_packet, 0, sizeof(rtlp_packet));
			memcpy(&rtlp_packet, packet_buf, SERVER_BUF_LEN);
			
		}
	}		

	close(socket_fd);
	
	return 0;
}
