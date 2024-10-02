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
	// Example commands: "/ping", "/help", "/time"
	if (strncmp(command, "/ping", 5) == 0) {

		char* space_pos = strchr(command, ' ');

		if (space_pos != NULL) {
	    	// Extract destination ID (in hexadecimal format).
    		char dest_id_str[4] = {0};  // Buffer for 2 hex digits plus null terminator

 		   // Make sure we are copying only the correct number of characters (2 hex digits).
    		size_t hex_len = space_pos - (command + 5);  // Length of the hex part
    		if (hex_len > 2) {
        		hex_len = 2;  // Limit to 2 characters (e.g., "ff")
    		}

    		strncpy(dest_id_str, command + 5, hex_len);  // Copy hex digits from command

    		// Convert to uint8_t
    		uint8_t destination_id = (uint8_t)strtol(dest_id_str, NULL, 16);  // Convert hex string to uint8

    		ping(destination_id);
		} else {
		    // Handle the case where there is no space in the command
    		printf("Error: Invalid command format.\n");
		}

	} else if (strcmp(command, "/help") == 0) {

		printf("Available commands: /ping, /help, /date\n");

	} else if (strcmp(command, "/date") == 0) {

		lownet_time_t current_time = lownet_get_time();

		if (current_time.seconds == 0) {
			printf("Network time is not available.\n");
		} else {
			printf("%lu.%u sec since the course started.\n", current_time.seconds, current_time.parts);
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

	// Extract destination ID (in hexadecimal format).
	char dest_id_str[4] = {0};
	strncpy(dest_id_str, input + 1, space_pos - input - 1);  // Skip '@' and get ID
	uint8_t destination_id = (uint8_t)strtol(dest_id_str, NULL, 16);  // Convert to uint8

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
