#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdint.h>
#include "../src/rtlp.h"

int main() {
	
	// Mocks a RTLP Packet and prints it.

	struct in_addr src_addr;
        struct in_addr dest_addr;
	inet_aton("192.168.0.2", &src_addr);
	inet_aton("192.168.0.3", &dest_addr);	

	uint8_t operation = RTLP_OPERATION_CLIENT_SENDALL; 

	uint8_t data[RTLP_DATA_LEN] = { 0 };
	strcpy(data, "teste");

	uint8_t type = RTLP_TYPE_CLIENT_TO_SERVER_REQ;

	uint8_t response = RTLP_RESPONSE_OK;
	
	uint8_t transport_protocol = RTLP_TRANSPORT_PROTOCOL_TCP;	
	
	struct rtlp_packet rtlp_packet;
	rtlp_packet_build(&rtlp_packet, &src_addr, &dest_addr, operation, data, type, response, transport_protocol);

	print_rtlp_packet(&rtlp_packet);
	return 0;
}
