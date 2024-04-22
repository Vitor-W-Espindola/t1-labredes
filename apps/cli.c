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
#include <sys/mman.h>

#include "../src/client.h"
#include "../src/server.h"

pthread_mutex_t client_mutex;

void die(char *s) {
	perror(s);
	exit(1);
}

int main(int argc, char **argv) {

	if(argc != 3) {
		printf("Usage: %s <hostname> <port>\n", argv[0]);
		exit(0);
	}

	// Initializes mutex
	if(pthread_mutex_init(&client_mutex, NULL) != 0) {
		printf("error: mutex init failed\n");
		return 1;	
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
	
	// Both parent and child processes hold a copy of the client structure
	void * shmem = mmap(NULL, sizeof(struct client), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	struct client * client = (struct client *) shmem;

	if(rtlp_packet.response == RTLP_RESPONSE_FULL_SERVER) printf("The server is full.\n");
	else {
		pid_t p;
		p = fork();
		if (p < 0) {
			close(socket_fd);
			exit(0);
		} else if (p == 0) {
			// Copy the default nickname sent by the server
			pthread_mutex_lock(&client_mutex);
			memcpy(client->nickname, rtlp_packet.data, CLIENT_NICKNAME_LEN);
			pthread_mutex_unlock(&client_mutex);

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
						else if(rtlp_packet.response == RTLP_RESPONSE_NEW_NICKNAME) {
							pthread_mutex_lock(&client_mutex);
							memcpy(client->nickname, rtlp_packet.data, CLIENT_NICKNAME_LEN);
							pthread_mutex_unlock(&client_mutex);
							printf("Your nickname has been changed.\n");
						} else if(rtlp_packet.response == RTLP_RESPONSE_LISTUSERS)
							printf("\n%s\n", rtlp_packet.data);
						else if(rtlp_packet.response == RTLP_RESPONSE_TRANSFERENABLED)
							printf("File transfering enable.\n");
						else if(rtlp_packet.response == RTLP_RESPONSE_TRANSFERDISABLED)
							printf("File transfering disable.\n");
						fflush(stdout);
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
				
				pthread_mutex_lock(&client_mutex);
				int from_command_to_packet_res = from_command_to_packet(cmd_buf, &rtlp_packet, client);
				pthread_mutex_unlock(&client_mutex);
				
				if(from_command_to_packet_res == -1) {
					printf("Wrong command.\n");
					continue;
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
				
				if(from_command_to_packet_res == 2) {
					// Quit
					kill(p, SIGKILL);
					break;
				}
			}
		}
	}		

	close(socket_fd);
	pthread_mutex_destroy(&client_mutex);
	
	return 0;
}
