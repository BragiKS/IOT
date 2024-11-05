#include "crypt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <esp_log.h>
#include <aes/esp_aes.h>

#include "serial_io.h"
#include "lownet.h"

void crypt_decrypt(const lownet_secure_frame_t* cipher, lownet_secure_frame_t* plain)
{
	const lownet_key_t* key = lownet_get_key();
    if (!key) {
        ESP_LOGE("CRYPT", "Decryption key not set.");
        return;
    }

    esp_aes_context aes;
    esp_aes_init(&aes);
    esp_aes_setkey(&aes, key->bytes, key->size * 8);

    // Copy unencrypted parts
    memcpy(plain->magic, cipher->magic, sizeof(cipher->magic));
    plain->source = cipher->source;
    plain->destination = cipher->destination;
    memcpy(plain->ivt, cipher->ivt, sizeof(cipher->ivt));

    // Decrypt payload using CBC mode
    esp_aes_crypt_cbc(&aes, ESP_AES_DECRYPT, LOWNET_PAYLOAD_SIZE, cipher->ivt, cipher->payload, plain->payload);

    // Copy remaining encrypted parts and CRC
    plain->protocol = cipher->protocol;
    plain->length = cipher->length;
    plain->crc = cipher->crc;

    esp_aes_free(&aes);
}

void crypt_encrypt(const lownet_secure_frame_t* plain, lownet_secure_frame_t* cipher)
{
	const lownet_key_t* key = lownet_get_key();
	if (!key) {
		ESP_LOGE("CRYPT", "Encryption key not set.");
		return;
	}

	esp_aes_context aes;
	esp_aes_init(&aes);
	esp_aes_setkey(&aes, key->bytes, key->size * 8);

	memcpy(cipher->magic, plain->magic, sizeof(plain->magic));
	cipher->source = plain->source;
	cipher->destination = plain->destination;
	memcpy(cipher->ivt, plain->ivt, sizeof(plain->ivt));

	// Encrypt payload using CBC mode
    esp_aes_crypt_cbc(&aes, ESP_AES_ENCRYPT, LOWNET_PAYLOAD_SIZE, plain->ivt, plain->payload, cipher->payload);

    // Copy remaining encrypted parts and CRC
    cipher->protocol = plain->protocol;
    cipher->length = plain->length;
    cipher->crc = plain->crc;

    esp_aes_free(&aes);
}

// Usage: crypt_command(KEY)
// Pre:   KEY is a valid AES key or NULL
// Post:  If key == NULL encryption has been disabled
//        Else KEY has been set as the encryption key to use for
//        lownet communication.
void crypt_setkey_command(char* args) 
{

	// Check if the key is "0" or "1" for predefined keys
    if (strcmp(args, "0") == 0) {
        lownet_key_t new_key = lownet_keystore_read(0);
        lownet_set_key(&new_key);
        serial_write_line("Keystore key 0 set.");
        return;
    }

    if (strcmp(args, "1") == 0) {
        lownet_key_t new_key = lownet_keystore_read(1);
        lownet_set_key(&new_key);
        serial_write_line("Keystore key 1 set.");
        return;
    }

	if (sizeof(args) > LOWNET_KEY_SIZE_AES) {
		serial_write_line("Too long key size");
		return;
	}
	

    // Create a buffer initialized to zeroes
    uint8_t padded_key[LOWNET_KEY_SIZE_AES] = {0};

    memcpy(padded_key, args, sizeof(args));

    // Set the key with zero-padded value if needed
    lownet_key_t new_key;
    new_key.bytes = padded_key;
    new_key.size = LOWNET_KEY_SIZE_AES;

    lownet_set_key(&new_key);
    serial_write_line("Encryption key set.");
}


void crypt_test_command(char* str)
{
	if (!str)
		return;
	if (!lownet_get_key())
		{
			serial_write_line("No encryption key set!");
			return;
		}

	// Encrypts and then decrypts a string, can be used to sanity check your
	// implementation.
	lownet_secure_frame_t plain;
	lownet_secure_frame_t cipher;
	lownet_secure_frame_t back;

	memset(&plain, 0, sizeof(lownet_secure_frame_t));
	memset(&cipher, 0, sizeof(lownet_secure_frame_t));
	memset(&back, 0, sizeof(lownet_secure_frame_t));

	const uint8_t cipher_magic[2] = {0x20, 0x4e};

	memcpy(plain.magic, cipher_magic, sizeof cipher_magic);
	plain.source = lownet_get_device_id();
	plain.destination = 0xFF;
	plain.protocol = LOWNET_PROTOCOL_CHAT;
	plain.length = strlen(str);

	*((uint32_t*) plain.ivt) = 123456789;
	strcpy((char*) plain.payload, str);

	crypt_encrypt(&plain, &cipher);

	if (memcmp(&plain, &cipher, LOWNET_UNENCRYPTED_SIZE) != 0)
		{
			serial_write_line("Unencrypted part of frame not preserved!");
			return;
		}
	if (memcmp(&plain.ivt, &cipher.ivt, LOWNET_IVT_SIZE) != 0)
		{
			serial_write_line("IVT not preserved!");
			return;
		}

	crypt_decrypt(&cipher, &back);

	if (memcmp(&plain, &back, sizeof plain) == 0)
		{
			serial_write_line("Encrypt/Decrypt successful");
			return;
		}

	serial_write_line("Encrypt/Decrypt failed");
	char msg[200];
	snprintf(msg, sizeof msg,
					 "Unencrypted content: %s\n"
					 "IVT:                 %s\n"
					 "Encrypted content:   %s\n",
					 memcmp(&plain, &back, LOWNET_UNENCRYPTED_SIZE) == 0 ? "Same" : "Different",
					 memcmp(&plain.ivt, &back.ivt, LOWNET_IVT_SIZE) == 0 ? "Same" : "Different",
					 memcmp(&plain.protocol, &back.protocol, LOWNET_ENCRYPTED_SIZE) == 0 ? "Same" : "Different"
	);
	serial_write_line(msg);
}
