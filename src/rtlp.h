// Description of the RTLP (Ride the Lightning Protocol)

#ifndef RTLP_H
#define RTLP_H

#include <stdint.h>
#include <arpa/inet.h>

#define RTLP_DATA_LEN 128

#define RTLP_OPERATION_SERVER_MSG 0
#define RTLP_OPERATION_CLIENT_SENDALL 1
#define RTLP_OPERATION_CLIENT_SENDPV 2
#define RTLP_OPERATION_CLIENT_TRANSFERPV 3
#define RTLP_OPERATION_CLIENT_NICKNAME 4
#define RTLP_OPERATION_CLIENT_QUIT 5

#define RTLP_TYPE_SERVER_TO_CLIENT_REQ 0
#define RTLP_TYPE_SERVER_TO_CLIENT_ACK 1
#define RTLP_TYPE_CLIENT_TO_SERVER_REQ 2
#define RTLP_TYPE_CLIENT_TO_SERVER_ACK 3

#define RTLP_RESPONSE_OK 0
#define RTLP_RESPONSE_NOK 1

#define RTLP_TRANSPORT_PROTOCOL_TCP 0
#define RTLP_TRANSPORT_PROTOCOL_UDP 1

// ...to be changed

// Structure of the RTLP packet
struct rtlp_packet {
	struct in_addr * src_addr;
	struct in_addr * dest_addr;
	uint8_t operation;
	uint8_t data[RTLP_DATA_LEN];
	uint8_t type;
	uint8_t response;
	uint8_t transport_protocol;
};

// In-place function for populating the rtlp_packet
uint8_t rtlp_packet_build(struct rtlp_packet * rtlp_packet, struct in_addr * src_addr, struct in_addr * dest_addr, uint8_t operation, uint8_t data[128], uint8_t type, uint8_t response, uint8_t transport_protocol);

// Function for visualizing data of RTLP packet
void print_rtlp_packet(struct rtlp_packet * rtlp_packet);

#endif
