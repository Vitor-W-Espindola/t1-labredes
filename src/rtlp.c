#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "rtlp.h"

uint8_t rtlp_packet_build(
		struct rtlp_packet * rtlp_packet, 
		uint8_t operation, 
		uint8_t source[RTLP_SOURCE_LEN],
		uint8_t destination[RTLP_DESTINATION_LEN],
		uint8_t data[RTLP_DATA_LEN],
		uint8_t type,
		uint8_t response, 
		uint8_t transport_protocol) {

	rtlp_packet->operation = operation;
	memset(rtlp_packet->source, 0, RTLP_SOURCE_LEN);
	memcpy(rtlp_packet->source, source, RTLP_SOURCE_LEN);
	memset(rtlp_packet->destination, 0, RTLP_DESTINATION_LEN);
	memcpy(rtlp_packet->destination, destination, RTLP_DESTINATION_LEN);
	memset(rtlp_packet->data, 0, RTLP_DATA_LEN);
	memcpy(rtlp_packet->data, data, RTLP_DATA_LEN);
	rtlp_packet->type = type;
	rtlp_packet->response = response;
	rtlp_packet->transport_protocol = transport_protocol;
	
	return 0;
}

void print_rtlp_packet(struct rtlp_packet * rtlp_packet) {
	printf("RTLP Packet\n");
	printf("operation: %d\nsource: %s\ndestination: %s\ndata: %s\ntype: %d\nresponse: %d\ntransport_protocol: %d\n", rtlp_packet->operation, rtlp_packet->source, rtlp_packet->destination, rtlp_packet->data, rtlp_packet->type, rtlp_packet->response, rtlp_packet->transport_protocol);
}
