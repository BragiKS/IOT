#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_ping.h"
#include "serial_io.h"
#include "lownet.h"

typedef struct __attribute__((__packed__))
{
	lownet_time_t timestamp_out;
	lownet_time_t timestamp_back;
	uint8_t origin;
} ping_packet_t;

// Function to send a ping to a specific node.
void ping(uint8_t node) {
	lownet_frame_t frame;
	memset(&frame, 0, sizeof(frame));

	frame.destination = node;  // Set the destination to the target node
	frame.protocol = LOWNET_PROTOCOL_PING;  // Define the protocol as PING
	frame.length = sizeof(ping_packet_t);  // The payload size is the size of the ping_packet_t structure

	ping_packet_t packet;
	memset(&packet, 0, sizeof(packet));

	// Set the initial timestamp and origin
	packet.timestamp_out = lownet_get_time();  // Capture the time when the ping is sent
	packet.origin = lownet_get_device_id();  // Set the origin to the current device's ID

	// Copy the ping packet to the frame's payload
	memcpy(frame.payload, &packet, sizeof(packet));

	// Send the ping packet via LowNet
	lownet_send(&frame);

	// Provide feedback for debugging
	printf("Ping sent to node 0x%02X\n", node);
}

// Function to handle incoming ping responses or ping requests.
void ping_receive(const lownet_frame_t* frame) {
	if (frame->length < sizeof(ping_packet_t)) {
		// Malformed frame.  Discard it.
		printf("Received malformed ping frame\n");
		return;
	}

	// Extract the ping packet from the frame
	ping_packet_t packet;
	memcpy(&packet, frame->payload, sizeof(packet));

	uint8_t current_node = lownet_get_device_id();

	// Check if this is a ping sent to us that we need to respond to
	if (frame->destination == current_node) {
		// This is a ping request directed to us.
		printf("Ping request received from node 0x%02X\n", packet.origin);

		// Prepare a ping response
		lownet_frame_t response_frame;
		memset(&response_frame, 0, sizeof(response_frame));

		// Fill the response frame with the necessary details
		response_frame.destination = packet.origin;  // Send it back to the origin of the ping
		response_frame.protocol = LOWNET_PROTOCOL_PING;  // Use the PING protocol
		response_frame.length = sizeof(ping_packet_t);  // Same length as the original ping packet

		// Copy the original timestamp out and add the return timestamp
		packet.timestamp_back = lownet_get_time();  // Timestamp when we send the response

		// Copy the updated packet back into the response frame
		memcpy(response_frame.payload, &packet, sizeof(packet));

		// Send the response back to the origin
		lownet_send(&response_frame);

		// Provide feedback for debugging
		printf("Ping response sent to node 0x%02X\n", packet.origin);
	} else if (frame->source == current_node) {
		// This is a ping response directed back to us.
		printf("Ping response received from node 0x%02X\n", packet.origin);

		// Calculate round-trip time
		lownet_time_t time_sent = packet.timestamp_out;
		lownet_time_t time_received = packet.timestamp_back;

		// Calculate round-trip time in milliseconds
		int64_t round_trip_ms = (int64_t)(time_received.seconds - time_sent.seconds) * 1000
							  + ((int64_t)(time_received.parts - time_sent.parts) * 1000) / 256;

		printf("Ping response from node 0x%02X: round-trip time = %lld ms\n", packet.origin, round_trip_ms);
	}
}
