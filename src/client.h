// Describes the operations that goes underly the client application

#ifndef CLIENT_H 
#define CLIENT_H

#include "../src/rtlp.h"

#define CLIENT_CMD_LEN 256
#define CLIENT_NICKNAME_LEN 16

extern const char * cmd_name_sendall;
extern const char * cmd_name_sendpv;
extern const char * cmd_name_transferpv;
extern const char * cmd_name_nickname;
extern const char * cmd_name_list;
extern const char * cmd_name_quit;

struct client {
	char nickname[CLIENT_NICKNAME_LEN];
};

struct file_manager_param {
	int server_socket_fd;
	struct rtlp_packet rtlp_packet;
};

// In-place function which processes a string (the full command line) and populates a RTLP packet.
int from_command_to_packet(char *command, struct rtlp_packet * rtlp_packet, struct client * client);
void * file_manager();

#endif
