#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "server.h"
#include "client.h"
#include "rtlp.h"

const char * cmd_name_sendall = "sendall";
const char * cmd_name_sendpv = "sendpv";
const char * cmd_name_transferpv = "transferpv";
const char * cmd_name_enabletransfer = "enabletransfer";
const char * cmd_name_disabletransfer = "disabletransfer";
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

	char cmd[CLIENT_CMD_LEN];
	memset(cmd, 0, CLIENT_CMD_LEN);
	
	// Reading name of the command and setting operation (or return failure)
	for(int i = 0; i < CLIENT_CMD_LEN; i++) {
		if(command[cmd_index] != ' ') cmd[i] = command[cmd_index++];
		else {
			cmd_index++;
			if(!strcmp(cmd, cmd_name_sendall))
				operation = RTLP_OPERATION_CLIENT_SENDALL; 
			else if(!strcmp(cmd, cmd_name_sendpv))
				operation = RTLP_OPERATION_CLIENT_SENDPV;
			else if(!strcmp(cmd, cmd_name_transferpv))
				operation = RTLP_OPERATION_CLIENT_TRANSFERPV;
			else if(!strcmp(cmd, cmd_name_enabletransfer))
				operation = RTLP_OPERATION_CLIENT_ENABLETRANSFER;
			else if(!strcmp(cmd, cmd_name_disabletransfer))
				operation = RTLP_OPERATION_CLIENT_DISABLETRANSFER;
			else if(!strcmp(cmd, cmd_name_nickname))
				operation = RTLP_OPERATION_CLIENT_NICKNAME;
			else if(!strcmp(cmd, cmd_name_list))
				operation = RTLP_OPERATION_CLIENT_LIST;
			else if(!strcmp(cmd, cmd_name_quit))
				operation = RTLP_OPERATION_CLIENT_QUIT;
			else return -1; 
			break;
		}
	}
	
	memset(cmd, 0, CLIENT_CMD_LEN);
	
	// Reading first parameter
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

	// Reading second parameter
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
		case RTLP_OPERATION_CLIENT_TRANSFERPV:
			// usage: transferpv <filename> <destination>
			// source -> source client's nickname
			// destination -> destination client's nickname
			// data -> filename and dir
			
			strcpy(source, client->nickname);
			strcpy(data, first_parameter);
			strcpy(destination, second_parameter);
			type = RTLP_TYPE_CLIENT_TO_SERVER_CHUNK;
			response = RTLP_RESPONSE_NONE;
			transport_protocol = RTLP_TRANSPORT_PROTOCOL_TCP;
			
			rtlp_packet_build(rtlp_packet, operation, source, destination, data, type, response, transport_protocol);
			return 1;			

			break;		
		case RTLP_OPERATION_CLIENT_ENABLETRANSFER:
			// usage: enabletransfer
			// source -> source client's nickname
			// destination -> empty
			// data -> empty

			strcpy(source, client->nickname);
			type = RTLP_TYPE_CLIENT_TO_SERVER_REQ;
			response = RTLP_RESPONSE_NONE;
			transport_protocol = RTLP_TRANSPORT_PROTOCOL_TCP;

			break;
		case RTLP_OPERATION_CLIENT_DISABLETRANSFER:
			// usage: disabletransfer
			// source -> source client's nickname
			// destination -> empty
			// data -> empty

			strcpy(source, client->nickname);
			type = RTLP_TYPE_CLIENT_TO_SERVER_REQ;
			response = RTLP_RESPONSE_NONE;
			transport_protocol = RTLP_TRANSPORT_PROTOCOL_TCP;

			break;
		case RTLP_OPERATION_CLIENT_NICKNAME:
			// usage: nickname <new_nickname>
			// source -> source client's nickname
			// destination -> empy
			// data -> new_nickname

			strcpy(source, client->nickname);
			strcpy(data, first_parameter); 
			type = RTLP_TYPE_CLIENT_TO_SERVER_REQ;
			response = RTLP_RESPONSE_NONE;
			transport_protocol = RTLP_TRANSPORT_PROTOCOL_UDP;
			break;
		default:
			break;
	}

	rtlp_packet_build(rtlp_packet, operation, source, destination, data, type, response, transport_protocol);
	
	return 0;
}

void * file_manager(void * file_manager_param) {
	struct file_manager_param * param = (struct file_manager_param *) file_manager_param;
	struct rtlp_packet rtlp_packet = param->rtlp_packet;
	FILE *fptr;
	fptr = fopen(rtlp_packet.data, "r");	
	if(fptr == NULL) {
		printf("Failed to open file %s\n", rtlp_packet.data);
		return NULL;
	}
	int user_socket_fd;

	char packet_buf[SERVER_BUF_LEN];

	while(1) {
		memset(rtlp_packet.data, 0, RTLP_DATA_LEN);
		int r_len = fread(&rtlp_packet.data, 1, RTLP_DATA_LEN, fptr);
		
		memset(packet_buf, 0, SERVER_BUF_LEN);
		memcpy(packet_buf, &rtlp_packet, SERVER_BUF_LEN);		
		
		int w_len = write(param->server_socket_fd, packet_buf, SERVER_BUF_LEN); 
		fflush(stdout);
		if(r_len < RTLP_DATA_LEN) break;
	}
}
