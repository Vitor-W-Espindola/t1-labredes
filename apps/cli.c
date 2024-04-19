#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>

#include "../src/client.h"
#include "../src/server.h"

void die(char *s) {
	perror(s);
	exit(1);
}

int main(int argc, char **argv) {

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
	if (recv_len == -1 && recv_len == 0) die("read"); 

	// Casts bytes to a rtlp_packet
	memset(&rtlp_packet, 0, sizeof(rtlp_packet));
	memcpy(&rtlp_packet, packet_buf, SERVER_BUF_LEN);	
	
	if(rtlp_packet.response == RTLP_RESPONSE_FULL_SERVER) printf("The server is full.\n");
	else {
		pid_t p;
		p = fork();
		if (p < 0) {
			close(socket_fd);
			exit(0);
		} else if (p == 0) {
			while(1) {
				// The child process manages incoming packets

				// Receives an ACK from server
				memset(packet_buf, 0, SERVER_BUF_LEN);
				
				int recv_len = read(socket_fd, packet_buf, SERVER_BUF_LEN);
				if (recv_len == -1 || recv_len == 0) die("read"); 

				// Casts bytes to a rtlp_packet
				memset(&rtlp_packet, 0, sizeof(rtlp_packet));
				memcpy(&rtlp_packet, packet_buf, SERVER_BUF_LEN);	

				switch(rtlp_packet.type) {
					case RTLP_TYPE_SERVER_TO_CLIENT_WELCOME:
						printf("%s\n", rtlp_packet.data);
						break;
					case RTLP_TYPE_SERVER_TO_CLIENT_ACK:
						if(rtlp_packet.response == RTLP_RESPONSE_USER_NOT_FOUND)
							printf("User not online.\n");
						else if(rtlp_packet.response == RTLP_RESPONSE_USER_NOT_AVAILABLE)
							printf("User is not available for file transfering.\n");
						break;					
					case RTLP_TYPE_SERVER_TO_CLIENT_PB_ASYNC:
						printf("\n(All) %s: %s\n", rtlp_packet.source, rtlp_packet.data);
						fflush(stdout);
						break;
					case RTLP_TYPE_SERVER_TO_CLIENT_PV_ASYNC:
						printf("\n(Private) %s: %s\n", rtlp_packet.source, rtlp_packet.data);
						fflush(stdout);
						break;
					case RTLP_TYPE_SERVER_TO_CLIENT_CHUNK:
						
						printf("\n(File) %s: %s\n", rtlp_packet.source, rtlp_packet.data);
						fflush(stdout);
						
						FILE *fptr;
						fptr = fopen("file.txt", "a+");
						if(fptr == NULL) break;

						fwrite(rtlp_packet.data, 1, RTLP_DATA_LEN, fptr);
						fclose(fptr);
						break;	
					default:
						break;
				}
			}
		} else {
			struct client client;
			strcpy(client.nickname, rtlp_packet.data);
			while(1) {

				// The parent process manages outcoming packets
				
				// Reads command from cli
				uint8_t cmd_buf[CLIENT_CMD_LEN];
				memset(cmd_buf, 0, CLIENT_CMD_LEN);
				
				fgets(cmd_buf, CLIENT_CMD_LEN, stdin);
				cmd_buf[strcspn(cmd_buf, "\r\n")] = ' ';
				cmd_buf[CLIENT_CMD_LEN - 1] = '\0';
				
				// Translate command to rtlp_packet
				memset(&rtlp_packet, 0, sizeof(rtlp_packet));
				int from_command_to_packet_res = from_command_to_packet(cmd_buf, &rtlp_packet, &client);
				if(from_command_to_packet_res == -1) {
					// Error
					kill(p, SIGKILL);
					printf("Error.\n");
					break;
				} else if(rtlp_packet.operation == RTLP_OPERATION_CLIENT_QUIT) {
					kill(p, SIGKILL);
					break;	
				} else if(from_command_to_packet_res == 1) {
					// File transfer thread	
					struct file_manager_param file_manager_param = {
						.server_socket_fd = socket_fd,
						.rtlp_packet = rtlp_packet 
					};
					pthread_t file_manager_thread_id;
					pthread_create(&file_manager_thread_id, NULL, file_manager, &file_manager_param);
					continue;
				}

				memset(packet_buf, 0, SERVER_BUF_LEN);
				memcpy(packet_buf, &rtlp_packet, sizeof(rtlp_packet));
				
				int write_len = write(socket_fd, packet_buf, SERVER_BUF_LEN);
				if (write_len < 0) die("write");
				
			}
		}
	}		

	close(socket_fd);
	
	return 0;
}
