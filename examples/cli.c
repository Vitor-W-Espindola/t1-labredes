#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../src/client.h"

int main() {
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
	
	struct rtlp_packet rtlp_packet;

	uint8_t cmd_buf[CLIENT_CMD_LEN];
	memset(cmd_buf, 0, CLIENT_CMD_LEN);
	fgets(cmd_buf, CLIENT_CMD_LEN, stdin);
	cmd_buf[strcspn(cmd_buf, "\r\n")] = ' ';
	cmd_buf[CLIENT_CMD_LEN - 1] = '\0';
	
	if(process_command(cmd_buf, &rtlp_packet) == 1) {
		printf("Invalid command.\n");
		return 1;
	};
	print_rtlp_packet(&rtlp_packet);

	return 0;
}
