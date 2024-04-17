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

int from_command_to_packet(char *command, struct rtlp_packet * rtlp_packet, struct client * client) {
	
	// Reading command
	uint8_t operation;
	
	uint8_t first_parameter[RTLP_DATA_LEN];
	uint8_t second_parameter[RTLP_DATA_LEN];

	memset(first_parameter, 0, RTLP_SOURCE_LEN);
	memset(second_parameter, 0, RTLP_DESTINATION_LEN);
	
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
	
	// Reading source parameter
	// Keeps words inside \' or \" as one single parameter
	uint8_t reading_message = 0;
	for(int i = 0; i < RTLP_DATA_LEN; i++) {
		if(command[cmd_index] != ' ') {
			if(command[cmd_index] == '\'' || command[cmd_index] == '\"') {
				if(reading_message == 0) reading_message = 1;
				else if(reading_message == 1) reading_message = 0;
				cmd_index++;
				i--;
				continue;
			}
			first_parameter[i] = command[cmd_index++];
		} else {
			cmd_index++;
			if (reading_message == 0) break;
			else first_parameter[i] = 0x20; // White space
		}
	}
	
	// Reading destination parameter
	for(int i = 0; i < RTLP_DATA_LEN; i++) {
		if(command[cmd_index] != ' ') {
			if(command[cmd_index] == '\'' || command[cmd_index] == '\"') {
				if(reading_message == 0) reading_message = 1;
				else if(reading_message == 1) reading_message = 0;
				cmd_index++;
				i--;
				continue;
			}
			second_parameter[i] = command[cmd_index++];
		} else {
			cmd_index++;
			if (reading_message == 0) break;
			else second_parameter[i] = 0x20;
		}
	}

	uint8_t source[RTLP_SOURCE_LEN];
	uint8_t destination[RTLP_DESTINATION_LEN];	
	uint8_t data[RTLP_DATA_LEN];
	uint8_t type, response, transport_protocol;

	memset(source, 0, RTLP_SOURCE_LEN);
	memset(destination, 0, RTLP_DESTINATION_LEN);
	memset(data, 0, RTLP_DATA_LEN);

	switch(operation) {
		case RTLP_OPERATION_CLIENT_SENDALL:
			// usage: sendall <message>
			// source -> client's nickname
			// destination -> nothing
			// data -> message
			
			strcpy(source, client->nickname);
			strcpy(data, first_parameter);
			type = RTLP_TYPE_CLIENT_TO_SERVER_ACK;
			response = RTLP_RESPONSE_NONE;
			transport_protocol = RTLP_TRANSPORT_PROTOCOL_TCP;
			
			break;
		case RTLP_OPERATION_CLIENT_SENDPV:
			// usage: sendpv <message> <destination>
			// source -> client's nickname
			// destination -> destination nickname
			// data -> message
		
			strcpy(source, client->nickname);
			strcpy(data, first_parameter);
			strcpy(destination, second_parameter);
			type = RTLP_TYPE_CLIENT_TO_SERVER_ACK;
			response = RTLP_RESPONSE_NONE;
			transport_protocol = RTLP_TRANSPORT_PROTOCOL_TCP;
		
			break;
		default:
			break;
	}

	rtlp_packet_build(rtlp_packet, operation, source, destination, data, type, response, transport_protocol);
	print_rtlp_packet(rtlp_packet);
	
	return 0;
}
