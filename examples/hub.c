#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../src/server.h"
#include "../src/rtlp.h"

void die(char *s) {
	perror(s);
	exit(1);
}

int main() {

	// Stores address and port info from server and client	
	struct sockaddr_in server_si, client_si;

	memset((char *) &server_si, 0, sizeof(server_si));
	memset((char *) &client_si, 0, sizeof(client_si));
	
	// Creates a file descriptor for a socket which works
	// with domain of IPV4 internet addresses (AF_INET),
	// connection-based byte streams (SOCK_STREAM), 
	// especifically TCP (IPPROTO_TCP).
	int server_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(server_socket_fd == -1) die("socket");

	// Binding configuration
	server_si.sin_family = AF_INET; // IPV4
	server_si.sin_port = htons(SERVER_PORT); // Translation to the Network Byte Order
	server_si.sin_addr.s_addr = htonl(INADDR_ANY); // For binding to all available network interfaces.

	// Names the socket by binding it
	int bind_res = bind(server_socket_fd, (struct sockaddr *) &server_si, sizeof(server_si));
	if(bind_res == -1) die("bind");

	// Makes a passive socket that only queue up 5 requests
	int listen_res = listen(server_socket_fd, 5);
	if(listen_res == -1) die("listen");

	// Keep listening for connections
	char buf[SERVER_BUF_LEN];
	int recv_len = 0;
	while (1) {
		memset(buf, 0, sizeof(buf));
		printf("Waiting a connection...");
		fflush(stdout);
		
		// A File Descriptor conn for the new connected socket
		// and for the size of the peer address structure
		int client_socket_fd;
		int client_addr_len = sizeof(client_si);
		client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client_si, &client_addr_len);
		if(client_socket_fd < 0) die("accept");

		printf("Client connected %s:%d\n", inet_ntoa(client_si.sin_addr), ntohs(client_si.sin_port));

		// Receives data from client, blocking the thread
		recv_len = read(client_socket_fd, buf, SERVER_BUF_LEN);
		if (recv_len == -1) die("read");

		
		struct rtlp_packet * rtlp_packet = (struct rtlp_packet *) &buf;
		
		printf("Data received\n");	
		
		print_rtlp_packet(rtlp_packet);
		for(int i = 0; i < SERVER_BUF_LEN; i++) 
			printf("%x ", buf[i]);
		

		close(client_socket_fd);	
	}	
	
	return 0;
}
