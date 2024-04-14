#include <string.h>

#include "client.h"
#include "rtlp.h"

const char * cmd_name_sendall = "sendall";
const char * cmd_name_sendpv = "sendpv";
const char * cmd_name_transferpv = "transferpv";
const char * cmd_name_transferpvaccept = "transferpvaccept";
const char * cmd_name_transferpvdecline = "transferpvdecline";
const char * cmd_name_nickname = "nickname";
const char * cmd_name_list = "list";
const char * cmd_name_quit = "quit";

int from_command_to_packet(char *command, struct rtlp_packet * rtlp_packet) {
	// E.g.: command = sendall <message>
	// return 0 -> packet populated successfully
	// return 1 -> invalid command

	uint8_t operation;
	uint8_t operation_first_parameter[RTLP_OPERATION_FIRST_PARAMETER_LEN];
	uint8_t operation_second_parameter[RTLP_OPERATION_SECOND_PARAMETER_LEN];

	memset(operation_first_parameter, 0, RTLP_OPERATION_FIRST_PARAMETER_LEN);
	memset(operation_second_parameter, 0, RTLP_OPERATION_SECOND_PARAMETER_LEN);
	
	int cmd_index = 0;

	char aux_buf[CLIENT_CMD_LEN];
	memset(aux_buf, 0, CLIENT_CMD_LEN);
	
	// Reading name of the command and setting operation (or returning failure)
	for(int i = 0; i < CLIENT_CMD_LEN; i++) {
		if(command[cmd_index] != ' ') aux_buf[i] = command[cmd_index++];
		else {
			cmd_index++;

			if(!strcmp(aux_buf, cmd_name_sendall))
				operation = RTLP_OPERATION_CLIENT_SENDALL; 
			else if(!strcmp(aux_buf, cmd_name_sendpv))
				operation = RTLP_OPERATION_CLIENT_SENDPV;
			else if(!strcmp(aux_buf, cmd_name_transferpv))
				operation = RTLP_OPERATION_CLIENT_TRANSFERPV;
			else if(!strcmp(aux_buf, cmd_name_transferpvaccept))
				operation = RTLP_OPERATION_CLIENT_TRANSFERPV_ACCEPT;
			else if(!strcmp(aux_buf, cmd_name_transferpvdecline))
				operation = RTLP_OPERATION_CLIENT_TRANSFERPV_DECLINE;
			else if(!strcmp(aux_buf, cmd_name_nickname))
				operation = RTLP_OPERATION_CLIENT_NICKNAME;
			else if(!strcmp(aux_buf, cmd_name_list))
				operation = RTLP_OPERATION_CLIENT_LIST;
			else if(!strcmp(aux_buf, cmd_name_quit))
				operation = RTLP_OPERATION_CLIENT_QUIT;
			else return 1; 
			break;
		}
	}

	memset(aux_buf, 0, CLIENT_CMD_LEN);
	
	// Reading 1st parameter of the command
	// Keeps words inside \' or \" as one single parameter
	uint8_t reading_message = 0;
	for(int i = 0; i < RTLP_OPERATION_FIRST_PARAMETER_LEN; i++) {
		if(command[cmd_index] != ' ') {
			if(command[cmd_index] == '\'' || command[cmd_index] == '\"') {
				if(reading_message == 0) reading_message = 1;
				else if(reading_message == 1) reading_message = 0;
				cmd_index++;
				i--;
				continue;
			}
			operation_first_parameter[i] = command[cmd_index++];
		} else {
			cmd_index++;
			if (reading_message == 0) break;
			else operation_first_parameter[i] = 0x20; // White space
		}
	}

	// Reading 2nd parameter of the command
	for(int i = 0; i < RTLP_OPERATION_SECOND_PARAMETER_LEN; i++) {
		if(command[cmd_index] != ' ') operation_second_parameter[i] = command[cmd_index++];
		else { cmd_index++; break; }
	}

	rtlp_packet_build(rtlp_packet, operation, operation_first_parameter, operation_second_parameter, RTLP_TYPE_CLIENT_TO_SERVER_REQ, RTLP_RESPONSE_OK, RTLP_TRANSPORT_PROTOCOL_TCP);
	
	return 0;
}
