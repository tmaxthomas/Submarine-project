#include "../common.h"
#include <SPI.h>
#include <strings.h>

/******************
 * DEFINES/MACROS *
 ******************/

#define CSN_PIN 3

/***********
 * GLOBALS *
 ***********/

struct in_pack_t in_packet;
struct out_pack_t out_packet;

/*************
 * FUNCTIONS *
 *************/

void SPI_write(uint8_t *buf, uint8_t len) {
    digitalWrite(CSN_PIN, LOW);
}

/**************
 * SETUP/LOOP *
 **************/

void setup() {
    Serial.begin(BAUD_RATE);
    // TODO: SPI mode might be wrong
    SPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CSN_PIN, HIGH);
}

void loop() {

    // TODO: Read the input packet from SPI, somehow

    Serial.write(&in_packet, sizeof(in_packet));

    Serial.readBytes(&out_packet, sizeof(out_packet));

    // TODO: Write the out packet to SPI, somehow
}
