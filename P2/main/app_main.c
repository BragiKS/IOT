// CSTDLIB includes.
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// LowNet includes.
#include "lownet.h"

#include "serial_io.h"
#include "utility.h"

#include "app_chat.h"
#include "app_ping.h"

#define MAX_COMMAND_ARGS 5  // Define maximum number of arguments for commands

const char* ERROR_OVERRUN = "ERROR // INPUT OVERRUN";
const char* ERROR_UNKNOWN = "ERROR // PROCESSING FAILURE";

const char* ERROR_COMMAND = "Command error";
const char* ERROR_ARGUMENT = "Argument error";

void app_frame_dispatch(const lownet_frame_t* frame) {
	switch(frame->protocol) {

	case LOWNET_PROTOCOL_RESERVE:
		// Invalid protocol, ignore.
		break;

	case LOWNET_PROTOCOL_TIME:
		// Not handled here.  Time protocol is special case
		break;

	case LOWNET_PROTOCOL_CHAT:
		chat_receive(frame);
		break;

	case LOWNET_PROTOCOL_PING:
		ping_receive(frame);
		break;

	default:
		// Unknown protocol, ignore.
		break;
	}
}


// Command parser for user input that starts with '/'.
void process_command(const char* command) {
	// Example commands: "/ping", "/help", "/date"
	if (strncmp(command, "/ping", 5) == 0) {

		char* space_pos = strchr(command, ' ');

		if (space_pos != NULL) {

    		// Convert to uint8_t
    		uint8_t destination_id = (uint8_t)strtol(command + 6, NULL, 16);  // Convert hex string to uint8
			printf("Ping destination: %d\n", destination_id);

    		ping(destination_id);
		} else {
		    // Handle the case where there is no space in the command
    		printf("Error: Invalid command format.\n");
		}

	} else if (strcmp(command, "/help") == 0) {

		printf("Available commands: /ping, /help, /date\n");

	} else if (strcmp(command, "/date") == 0) {

		lownet_time_t current_time = lownet_get_time();
		if (current_time.seconds == 0 && current_time.parts == 0) {
			printf("Network time is not available.\n");
		} else {
			uint32_t parts = current_time.parts * 10 / 256;
			printf("%lu.%lu sec since the course started.\n", current_time.seconds, parts);
		}

	} else {
		printf("%s: %s\n", ERROR_COMMAND, command);
	}
}

// Parse and send 'tell' (direct message).
void process_tell(const char* input) {
	// Input format: "@<destination_id> <message>"
	char* space_pos = strchr(input, ' ');
	if (space_pos == NULL) {
		printf("%s: No message provided.\n", ERROR_ARGUMENT);
		return;
	}

	uint8_t destination_id = (uint8_t)strtol(input + 1, NULL, 16);  // Convert to uint8
	printf("Destination ID: %d\n", destination_id);

	// Extract message after destination ID.
	const char* message = space_pos + 1;

	// Check if message is valid.
	if (strlen(message) == 0) {
		printf("%s: No message provided.\n", ERROR_ARGUMENT);
		return;
	}

	// Send the direct message (tell).
	chat_tell(message, destination_id);
}

void app_main(void) {
	char msg_in[MSG_BUFFER_LENGTH];
	char msg_out[MSG_BUFFER_LENGTH];

	// Initialize serial and LowNet services.
	init_serial_service();
	lownet_init(app_frame_dispatch);

	while (true) {
		memset(msg_in, 0, MSG_BUFFER_LENGTH);
		memset(msg_out, 0, MSG_BUFFER_LENGTH);

		// Wait for a line of input.
		if (!serial_read_line(msg_in)) {
			if (msg_in[0] == '/') {
				// Process as a command.
				process_command(msg_in);
			} else if (msg_in[0] == '@') {
				// Process as a tell (direct message).
				process_tell(msg_in);
			} else {
				// Default to chat broadcast (shout).
				chat_shout(msg_in);
			}
		}
	}
}
