// Description of the RTLP (Ride the Lightning Protocol)

#ifndef RTLP_H
#define RTLP_H

#include <stdint.h>
#include <arpa/inet.h>

#define RTLP_OPERATION_FIRST_PARAMETER_LEN 256
#define RTLP_OPERATION_SECOND_PARAMETER_LEN 256

#define RTLP_OPERATION_SERVER_MSG 0x0
#define RTLP_OPERATION_CLIENT_SENDALL 0x1
#define RTLP_OPERATION_CLIENT_SENDPV 0x2
#define RTLP_OPERATION_CLIENT_TRANSFERPV 0x3
#define RTLP_OPERATION_CLIENT_TRANSFERPV_ACCEPT 0x4
#define RTLP_OPERATION_CLIENT_TRANSFERPV_DECLINE 0x5
#define RTLP_OPERATION_CLIENT_NICKNAME 0x6
#define RTLP_OPERATION_CLIENT_LIST 0x7
#define RTLP_OPERATION_CLIENT_QUIT 0x8

#define RTLP_TYPE_SERVER_TO_CLIENT_REQ 0x0
#define RTLP_TYPE_SERVER_TO_CLIENT_ACK 0x1
#define RTLP_TYPE_CLIENT_TO_SERVER_REQ 0x2
#define RTLP_TYPE_CLIENT_TO_SERVER_ACK 0x3

#define RTLP_RESPONSE_OK 0x0
#define RTLP_RESPONSE_NOK 0x1

#define RTLP_TRANSPORT_PROTOCOL_TCP 0x0
#define RTLP_TRANSPORT_PROTOCOL_UDP 0x1

// ...to be changed

// Structure of the RTLP packet
struct rtlp_packet {
	uint8_t operation;
	uint8_t operation_first_parameter[RTLP_OPERATION_FIRST_PARAMETER_LEN];
	uint8_t operation_second_parameter[RTLP_OPERATION_SECOND_PARAMETER_LEN];
	uint8_t type;
	uint8_t response;
	uint8_t transport_protocol;
};

// In-place function for populating the rtlp_packet
uint8_t rtlp_packet_build(struct rtlp_packet * rtlp_packet, uint8_t operation, uint8_t operation_first_parameter[RTLP_OPERATION_FIRST_PARAMETER_LEN], uint8_t operation_second_parameter[RTLP_OPERATION_SECOND_PARAMETER_LEN], uint8_t type, uint8_t response, uint8_t transport_protocol);

// Function for visualizing data of RTLP packet
void print_rtlp_packet(struct rtlp_packet * rtlp_packet);

#endif
