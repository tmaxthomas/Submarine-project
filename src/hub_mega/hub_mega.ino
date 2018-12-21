#include <Wire.h>
#include "../common.h"

#define D_MILS 50 //Length of cycle, in milliseconds

typedef struct {
    uint16_t pos, set, old;
    bool dir;
} encoder_t spool, shaft, ballast, shuttle;

static inline void increment(encoder_t *enc, uint8_t val) {
    uint8_t delta = val - enc->old;
    enc->old = val;
    enc->pos += (enc->dir ? delta : -delta);
}

static inline void pwm_write(uint8_t addr, uint8_t val) {
    Wire.beginTransmission(PWM_ADDR);
    Wire.write(addr);
    Wire.write(val);
    Wire.endTransmission();
}

// I2C address of the PWM controller
#define PWM_ADDR 0x40

struct in_pack in_packet;
struct out_pack out_packet;

float shaft_speed;

// Other sensor/misc data
uint8_t flooded;
int8_t shaft_voltage;
uint16_t fore_plane_set, aft_plane_set, rudder_set;

// PID struct
struct pid_t {
    float p, i, d;
    float total_err, old_val;
} pid[2];

// Updates PID controller and returns updated motor power percentage
// NOTE: Loops need to be tuned to produce values in the range 0 : 16,383
uint16_t update_pid(struct pid_t c, uint16_t set_pt, uint16_t val) {
    float err = set_pt - val;
    c.total_err += err;
    float d_val = c.old_val - val;
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
void read_encoders() {
    // Pins 22 (PORTA 0) to 29 (PORTA 7)
    uint8_t shaft_raw = PORTA;
    increment(&shaft, shaft_raw);

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
    uint8_t data[5] = {0x06 + 4*num, 0, 0, val, val>>8};
    Wire.write(data, 5);
    Wire.endTransmission();
}

// Emergency abort routine
// (just shuts down the Mega and lets the backup controller do its thing)
void emerg_abort() {
    for(;;);
}

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

uint32_t old_mils;

void loop() {
    old_mils = millis();

    uint8_t checksum = 0;

    // Recieve packet from groundstation
    Serial.readBytes(in_pack_buf, IN_PACKET_SIZE);

    // Confirm packet integrity
    for (int i = 0; i < IN_PACKET_SIZE; i++) {
        checksum ^= in_pack_buf[i];
    }

    if (!checksum) {
        shaft_voltage = in_pack_buf[0];
        ballast.set = *(uint16_t *) (in_pack_buf + 4);
        spool.set = *(uint16_t *) (in_pack_buf + 6);
        fore_plane_set = in_pack_buf[1] << 6;
        aft_plane_set = in_pack_buf[2] << 6;
        rudder_set = in_pack_buf[3] << 6;
    }

    // TODO: Determine if this is actually a todo
    // TODO: Redo flood determination code
    // Read flooding data and act accordingly
    flooded = PORTG;
    if (flooded) emerg_abort();

    read_encoders();

    pwm_update(0, shaft_voltage << 8);
    pwm_update(1, update_pid(pid[0], ballast_set, ballast_pos));
    pwm_update(2, fore_plane_set); //Fore dive planes
    pwm_update(3, aft_plane_set); //Aft dive planes
    pwm_update(4, rudder_set); //Aft rudder
    pwm_update(5, update_pid(pid[1], spool_set, spool_pos));

    // TODO: Figure out what the heck we're doing with the running lights


    // Populate packet with data
    out_packet.shaft_speed = shaft_speed;
    out_packet.ballast_pos = ballast_pos;
    out_packet.spool_pos = spool_pos;
    out_packet.flooded = flooded;
    out_packet.shaft_voltage = shaft_voltage;

    // Compute and append checksum
    out_packet.checksum = 0;
    for (uint8_t i = 0; i < sizeof(out_packet) - 1; i++) {
        checksum ^= ((uint8_t *) &out_packet)[i];
    }

    Serial.write(&out_packet, sizeof(out_packet));

    // Yes, this hammers the CPU... but since we're one process, running in real mode, it doesn't
    // matter
    while (millis() - old_mils < D_MILS); {
        old_mils = millis();
    }
}
