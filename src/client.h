#ifndef CLIENT_H 
#define CLIENT_H

#include "../src/rtlp.h"

// Describes the operations that goes underly the client application

#define CLIENT_CMD_LEN 256

extern const char * cmd_name_sendall;
extern const char * cmd_name_sendpv;
extern const char * cmd_name_transferpv;
extern const char * cmd_name_nickname;
extern const char * cmd_name_list;
extern const char * cmd_name_quit;

// In-place function which processes a string (the full command line) and populates a RTLP packet.
int process_command(char *command, struct rtlp_packet * rtlp_packet);

#endif
