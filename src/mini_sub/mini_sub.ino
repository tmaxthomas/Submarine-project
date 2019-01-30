#include "../common.h"
#include "../RF24/RF24.h"

/******************
 * DEFINES/MACROS *
 ******************/

// TODO: Check the actual pinout
#define CE_PIN 4
#define CS_PIN 3

/***********
 * GLOBALS *
 ***********/

struct in_pack_t in_packet;
struct out_pack_t out_packet;

RF_24 radio(CE_PIN, CS_PIN, SPI_RATE);

/**************
 * SETUP/LOOP *
 **************/

void setup() {
    Serial.begin(115200);

    radio.begin();
    radio.openWritingPipe(GROUNDSTATION_ADDR);
    radio.openReadingPipe(MINISUB_ADDR);
    radio.startListening();
}

void loop() {
    // Busy loop until something is available
    while (!radio.available());

    // Read the thing
    radio.read(&in_packet, sizeof(in_packet));

    // Write the thing to the hub
    Serial.write(&in_packet, sizeof(in_packet));

    // Read the hub's response
    Serial.readBytes(&out_packet, sizeof(out_packet));

    // Send the hub's response to the groundstation
    radio.stopListening();
    radio.write(&out_packet, sizeof(out_packet));
    radio.startListening();
}
