#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "rtlp.h"

uint8_t rtlp_packet_build(
		struct rtlp_packet * rtlp_packet, 
		uint8_t operation, 
		uint8_t operation_first_parameter[RTLP_OPERATION_FIRST_PARAMETER_LEN],
		uint8_t operation_second_parameter[RTLP_OPERATION_SECOND_PARAMETER_LEN],
		uint8_t type,
		uint8_t response, 
		uint8_t transport_protocol) {

	rtlp_packet->operation = operation;
	memset(rtlp_packet->operation_first_parameter, 0, RTLP_OPERATION_FIRST_PARAMETER_LEN);
	memcpy(rtlp_packet->operation_first_parameter, operation_first_parameter, RTLP_OPERATION_FIRST_PARAMETER_LEN);
	memset(rtlp_packet->operation_second_parameter, 0, RTLP_OPERATION_SECOND_PARAMETER_LEN);
	memcpy(rtlp_packet->operation_second_parameter, operation_second_parameter, RTLP_OPERATION_SECOND_PARAMETER_LEN);
	rtlp_packet->type = type;
	rtlp_packet->response = response;
	rtlp_packet->transport_protocol = transport_protocol;
	
	return 0;
}

void print_rtlp_packet(struct rtlp_packet * rtlp_packet) {
	printf("RTLP Packet\n");
	printf("operation: %d\noperation_first_parameter:%s \noperation_second_parameter:%s \ntype: %d\nresponse: %d\ntransport_protocol: %d\n", rtlp_packet->operation, rtlp_packet->operation_first_parameter, rtlp_packet->operation_second_parameter, rtlp_packet->type, rtlp_packet->response, rtlp_packet->transport_protocol);
}
