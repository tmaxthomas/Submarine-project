#include <Wire.h>
#include "../common.h"

/******************
 * MACROS/DEFINES *
 ******************/

//Length of cycle, in milliseconds
#define D_MILS 50
// I2C address of the PWM controller
#define PWM_ADDR 0x40

/***********
 * STRUCTS *
 ***********/

struct encoder_t {
    uint16_t pos, set, old;
    bool dir;
} spool, ballast, shuttle;

struct pid_t {
    uint16_t p, i, d;
    uint16_t total_err, old_val;
} pid[2];

/***********
 * GLOBALS *
 ***********/

struct in_pack_t in_packet;
struct out_pack_t out_packet;

// Other sensor/misc data
uint8_t flooded;
int8_t shaft_voltage;
uint16_t fore_plane_set, aft_plane_set, rudder_set;
uint32_t old_mils;

/*************
 * FUNCTIONS *
 *************/

void increment(struct encoder_t *enc, uint8_t val) {
    uint8_t delta = val - enc->old;
    enc->old = val;
    enc->pos += (enc->dir ? delta : -delta);
}

void pwm_write(uint8_t addr, uint8_t val) {
    Wire.beginTransmission(PWM_ADDR);
    Wire.write(addr);
    Wire.write(val);
    Wire.endTransmission();
}

// Updates PID controller and returns updated motor power percentage
// NOTE: Loops need to be tuned to produce values in the range 0 : 16,383
uint16_t update_pid(struct pid_t c, uint16_t set_pt, uint16_t val) {
    uint16_t err = set_pt - val;
    c.total_err += err;
    uint16_t d_val = c.old_val - val;
    c.old_val = val;
    return (err * c.p) + (c.total_err * c.i) - (d_val * c.d);
}

// Runs initialization code for the PWM controller
void pwm_controller_init() {
    // Put the controller to sleep in order to prep for prescale
    pwm_write(0x00, 0x10);

    //Write prescale
    pwm_write(0xFE, 0x79);

    // Wake the controller back up and turn auto-increment on
    pwm_write(0x00, 0x20);
}

// Reads and records updated encoder values off of the parallel busses coming from
// the encoder Nanos
inline void read_encoders() {
    // Pins 34 (PORTC 3) to 30 (PORTC 7) and 49 (PORTL 0) to 46 (PORTL 3)
    uint8_t ballast_raw, spool_raw, shuttle_raw;
    uint16_t aggregated_raw = ((PORTC & 0b11111000) << 1) + (PORTL & 0b1111);
    ballast_raw = aggregated_raw & 0b111;
    spool_raw = (aggregated_raw >> 3) & 0b111;
    shuttle_raw = (aggregated_raw >> 6) & 0b111;

    increment(&ballast, ballast_raw);
    increment(&spool, spool_raw);
    increment(&shuttle, shuttle_raw);
}

void pwm_update(uint8_t num, uint16_t val) {
    val = (val < 4095) ? val : 4096;

    // Write a whole bunch of magic
    // Refer to the PCA9685 datasheet for more information
    Wire.beginTransmission(PWM_ADDR);
    uint8_t data[5] = {0x06 + 4*num, 0, 0, val, val >> 8};
    Wire.write(data, 5);
    Wire.endTransmission();
}

// Emergency abort routine
// (just shuts down the Mega and lets the backup controller do its thing)
void emerg_abort() {
    for(;;);
}

/**************
 * SETUP/LOOP *
 **************/

void setup() {
    Serial.begin(BAUD_RATE);     //USB
    Serial1.begin(BAUD_RATE);    //Radio/mini-sub

    pid[0] = { .p = 0.1, .i = 0, .d = 0 }; //Ballast PID
    pid[1] = { .p = 0.1, .i = 0, .d = 0 }; //Spool PID

    // Register config
    DDRA &= 0b00000000;
    DDRC &= 0b00000000;
    DDRG &= 0b00000111;
    DDRL &= 0b00000000;
}

void loop() {
    old_mils = millis();

    // Recieve packet from groundstation
    Serial.readBytes(&in_packet, sizeof(in_packet));

    // Confirm packet integrity
    uint8_t checksum_verify = 0,
            *pack_ptr = (uint8_t *) &in_packet;
    for (int i = 0; i < sizeof(in_packet); i++) {
        checksum_verify ^= pack_ptr[i];
    }

    if (!checksum_verify) {
        shaft_voltage = in_packet.shaft_voltage;
        ballast.set = in_packet.ballast_set;
        spool.set = in_packet.spool_set;
        fore_plane_set = in_packet.fore_plane_set;
        aft_plane_set = in_packet.aft_plane_set;
        rudder_set = in_packet.rudder_set;
    }

    // TODO: Determine if this is actually a todo
    // TODO: Redo flood determination code
    // Read flooding data and act accordingly
    flooded = PORTG;
    if (flooded) emerg_abort();

    read_encoders();

    pwm_update(0, shaft_voltage << 4);
    pwm_update(1, update_pid(pid[0], ballast_set, ballast_pos));
    pwm_update(2, fore_plane_set << 4); //Fore dive planes
    pwm_update(3, aft_plane_set << 4); //Aft dive planes
    pwm_update(4, rudder_set << 4); //Aft rudder
    pwm_update(5, update_pid(pid[1], spool_set, spool_pos));

    // TODO: Figure out what the heck we're doing with the running lights


    // Populate packet with data
    out_packet.ballast_pos = ballast_pos;
    out_packet.spool_pos = spool_pos;
    out_packet.flooded = flooded;
    out_packet.shaft_voltage = shaft_voltage;

    // Compute and append checksum
    out_packet.checksum = 0;
    pack_ptr = (uint8_t *) &out_packet;
    for (uint8_t i = 0; i < sizeof(out_packet) - 1; i++) {
        out_packet.checksum ^= pack_ptr[i];
    }

    Serial.write(&out_packet, sizeof(out_packet));

    // Yes, this hammers the CPU... but since we're one process, running in real mode, it doesn't
    // matter
    int new_mils;
    while ((new_mils = millis()) - old_mils < D_MILS) {
        old_mils = new_mils;
    }
}
