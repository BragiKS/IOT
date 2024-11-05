#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <string.h>

#include "lownet.h"
#include "serial_io.h"

#include "app_chat.h"

void chat_receive(const lownet_frame_t* frame) {
    if (frame->destination == lownet_get_device_id()) {
        // This is a tell message, just for us!
        char received_message[LOWNET_PAYLOAD_SIZE + 1];  // LOWNET_PAYLOAD_SIZE defined in lownet.h
        memcpy(received_message, frame->payload, frame->length);  // Copy the message payload
        received_message[frame->length] = '\0';  // Null terminate

        printf("Received a tell from 0x%02X %s\n", frame->source, received_message);

    } else if (frame->destination == 0xFF) {
        // This is a broadcast shout message.
        char broadcast_message[LOWNET_PAYLOAD_SIZE + 1];
        memcpy(broadcast_message, frame->payload, frame->length);
        broadcast_message[frame->length] = '\0';  // Null terminate

        printf("Received a shout from 0x%02X: %s\n", frame->source, broadcast_message);

    }
}

void chat_shout(const char* message) {
    lownet_frame_t frame;
    memset(&frame, 0, sizeof(lownet_frame_t));
    
    frame.source = lownet_get_device_id();
    frame.destination = 0xFF;  // Use the broadcast node address
    frame.protocol = LOWNET_PROTOCOL_CHAT;  // Assuming LOWNET_PROTOCOL_CHAT is defined for chat messages
    frame.length = strlen(message);

    if (frame.length > LOWNET_PAYLOAD_SIZE) {
        printf("Error: Message is too long to send\n");
        return;
    }

    memcpy(frame.payload, message, frame.length);  // Copy the message into the payload

    lownet_send(&frame);  // Send the broadcast shout message

    printf("Shouted message: %s\n", message);
}

void chat_tell(const char* message, uint8_t destination) {
    lownet_frame_t frame;
    memset(&frame, 0, sizeof(lownet_frame_t));

    frame.source = lownet_get_device_id();
    frame.destination = destination;  // Send to a specific destination
    frame.protocol = LOWNET_PROTOCOL_CHAT;  // Assuming chat protocol
    frame.length = strlen(message);

    if (frame.length > LOWNET_PAYLOAD_SIZE) {
        printf("Error: Message is too long to send\n");
        return;
    }

    memcpy(frame.payload, message, frame.length);  // Copy the message into the payload

    lownet_send(&frame);  // Send the direct tell message

    printf("Sent tell message to device 0x%02X: %s\n", destination, message);
}