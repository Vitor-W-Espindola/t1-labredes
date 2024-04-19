// Description of the RTLP (Ride the Lightning Protocol)

#ifndef RTLP_H
#define RTLP_H

#include <stdint.h>
#include <arpa/inet.h>

#define RTLP_OPERATION_SERVER_MSG 0x0
#define RTLP_OPERATION_CLIENT_SENDALL 0x1
#define RTLP_OPERATION_CLIENT_SENDPV 0x2
#define RTLP_OPERATION_CLIENT_TRANSFERPV 0x3
#define RTLP_OPERATION_CLIENT_ENABLETRANSFER 0x4
#define RTLP_OPERATION_CLIENT_DISABLETRANSFER 0x5
#define RTLP_OPERATION_CLIENT_NICKNAME 0x6
#define RTLP_OPERATION_CLIENT_LIST 0x7
#define RTLP_OPERATION_CLIENT_QUIT 0x8

#define RTLP_SOURCE_LEN 64
#define RTLP_DESTINATION_LEN 64
#define RTLP_DATA_LEN 512

#define RTLP_TYPE_SERVER_TO_CLIENT_WELCOME 0x0
#define RTLP_TYPE_SERVER_TO_CLIENT_REQ 0x1
#define RTLP_TYPE_SERVER_TO_CLIENT_ACK 0x2
#define RTLP_TYPE_CLIENT_TO_SERVER_REQ 0x3
#define RTLP_TYPE_CLIENT_TO_SERVER_ACK 0x4
#define RTLP_TYPE_CLIENT_TO_SERVER_CHUNK 0x5
#define RTLP_TYPE_SERVER_TO_CLIENT_CHUNK 0x6
#define RTLP_TYPE_SERVER_TO_CLIENT_PV_ASYNC 0x7
#define RTLP_TYPE_SERVER_TO_CLIENT_PB_ASYNC 0x8

#define RTLP_RESPONSE_NONE 0x0
#define RTLP_RESPONSE_OK 0x1
#define RTLP_RESPONSE_NOK 0x2
#define RTLP_RESPONSE_FULL_SERVER 0x3
#define RTLP_RESPONSE_USER_NOT_FOUND 0x4
#define RTLP_RESPONSE_USER_NOT_AVAILABLE 0x5
#define RTLP_RESPONSE_NICKNAME_TOO_LONG 0x6

#define RTLP_TRANSPORT_PROTOCOL_TCP 0x0
#define RTLP_TRANSPORT_PROTOCOL_UDP 0x1

// ...to be changed

// Structure of the RTLP packet
struct rtlp_packet {
	// TODO: change for parameters: source, destination, data

	uint8_t operation;
	uint8_t source[RTLP_SOURCE_LEN];
	uint8_t destination[RTLP_DESTINATION_LEN];
	uint8_t data[RTLP_DATA_LEN];
	uint8_t type;
	uint8_t response;
	uint8_t transport_protocol;
};

// In-place function for populating the rtlp_packet
uint8_t rtlp_packet_build(struct rtlp_packet * rtlp_packet, uint8_t operation, uint8_t source[RTLP_SOURCE_LEN], uint8_t destination[RTLP_DESTINATION_LEN], uint8_t data[RTLP_DATA_LEN], uint8_t type, uint8_t response, uint8_t transport_protocol);

// Function for visualizing data of RTLP packet
void print_rtlp_packet(struct rtlp_packet * rtlp_packet);

#endif
