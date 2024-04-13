#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "rtlp.h"

uint8_t rtlp_packet_build(
		struct rtlp_packet * rtlp_packet, 
		struct in_addr * src_addr, 
		struct in_addr * dest_addr, 
		uint8_t operation, 
		uint8_t data[RTLP_DATA_LEN], 
		uint8_t type,
		uint8_t response, 
		uint8_t transport_protocol) {

	rtlp_packet->src_addr = src_addr;
	rtlp_packet->dest_addr = dest_addr;
	rtlp_packet->operation = operation;
	memset(rtlp_packet->data, 0, RTLP_DATA_LEN);
	memcpy(rtlp_packet->data, data, RTLP_DATA_LEN);
	rtlp_packet->type = type;
	rtlp_packet->response = response;
	rtlp_packet->transport_protocol = transport_protocol;
	
	return 0;
}

void print_rtlp_packet(struct rtlp_packet * rtlp_packet) {
	printf("RTLP Packet\n");
	printf("src_addr: %lu\ndest_addr: %lu\noperation: %d\ndata: %s\ntype: %d\nresponse: %d\ntransport_protocol: %d\n", rtlp_packet->src_addr->s_addr, rtlp_packet->dest_addr->s_addr, rtlp_packet->operation, rtlp_packet->data, rtlp_packet->type, rtlp_packet->response, rtlp_packet->transport_protocol);
}
