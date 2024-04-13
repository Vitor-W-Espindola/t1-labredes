#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "../src/rtlp.h"

int main() {
	
	// Mocks a RTLP Packet and prints it.


	uint8_t operation = RTLP_OPERATION_CLIENT_SENDALL; 
	
	uint8_t operation_first_parameter[RTLP_OPERATION_FIRST_PARAMETER_LEN];
	memset(operation_first_parameter, 0,  RTLP_OPERATION_FIRST_PARAMETER_LEN);
	strcpy(operation_first_parameter, "eae. beleza?");

	uint8_t operation_second_parameter[RTLP_OPERATION_SECOND_PARAMETER_LEN];
	memset(operation_second_parameter, 0, RTLP_OPERATION_SECOND_PARAMETER_LEN);

	uint8_t type = RTLP_TYPE_CLIENT_TO_SERVER_REQ;

	uint8_t response = RTLP_RESPONSE_OK;
	
	uint8_t transport_protocol = RTLP_TRANSPORT_PROTOCOL_TCP;	
	
	struct rtlp_packet rtlp_packet;
	rtlp_packet_build(&rtlp_packet, operation, operation_first_parameter, operation_second_parameter, type, response, transport_protocol);

	print_rtlp_packet(&rtlp_packet);
	return 0;
}
