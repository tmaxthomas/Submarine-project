#include "../common.h"
#include "../RF24/RF24.h"

/******************
 * DEFINES/MACROS *
 ******************/

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
    radio.openWritingPipe(MINISUB_ADDR);
    radio.openReadingPing(GROUNDSTATION_ADDR);
}

void loop() {
    // Read a packet from the groundstation
    Serial.readBytes(&in_packet, sizeof(in_packet));

    // Send the packet to the sub
    radio.write(&in_packet, sizeof(in_packet));

    // Get the response
    radio.startListening();
    while (!radio.available());
    radio.read(&out_packet, sizeof(out_packet));
    radio.stopListening();

    // Send the rcvd packet to the groundstation
    Serial.write(&out_packet, sizeof(out_packet));
}
