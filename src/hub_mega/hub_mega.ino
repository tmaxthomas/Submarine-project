#include <stdint.h>
#include <Wire.h>

#include "../common.h"

#define D_MILS 20 //Length of cycle, in milliseconds

// Macro for updating encoder positions, accounting for direction of rotation
#define INCREMENT(VAR, VAL)                                 \
    do {                                                    \
        uint8_t COMP ## _new = VAL, delta = 0;              \
        delta = COMP ## _new - COMP ## _old;                \
        COMP ## _old = COMP ## _new;                        \
        COMP ## _pos += ( COMP ## _dir ? delta : -delta );  \
    } while(0)

// Macro for writing packets to the PWM controller over I2C
#define PWM_WRITE(addr, val)                                \
    do {                                                    \
        Wire.beginTransmission(PWM_ADDR);                   \
        Wire.write(addr);                                   \
        Wire.write(val);                                    \
        Wire.endTransmission();                             \
    } while(0)

// I2C address of the PWM controller
#define PWM_ADDR 0x40

uint8_t in_pack_buf[IN_PACKET_SIZE], out_pack_buf[OUT_PACKET_SIZE];

// Encoder data
uint16_t spool_pos, shaft_pos, ballast_pos, shuttle_pos;
uint16_t spool_set, ballast_set, shuttle_set;
uint8_t spool_old, shaft_old, ballast_old, shuttle_old;
bool spool_dir, ballast_dir, shaft_dir, shuttle_dir;
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
    //Put the controller to sleep in order to prep for prescale
    PWM_WRITE(0x00, 0x10);

    //Write prescale
    PWM_WRITE(0xFE, 0x79);

    //Wake the controller back up and turn auto-increment on
    PWM_WRITE(0x00, 0x20);
}

// Reads and records updated encoder values off of the parallel busses coming from
// the encoder Nanos
void read_encoders() {
    // Pins 22 (PORTA 0) to 29 (PORTA 7)
    uint8_t shaft_raw = PORTA;
    INCREMENT(shaft, shaft_raw);

    // Pins 34 (PORTC 3) to 30 (PORTC 7) and 49 (PORTL 0) to 46 (PORTL 3)
    uint8_t ballast_raw, spool_raw, shuttle_raw;
    uint16_t aggregated_raw = ((PORTC & 0b11111000) << 1) + (PORTL & 0b1111);
    ballast_raw = aggregated_raw & 0b111;
    spool_raw = (aggregated_raw >> 3) & 0b111;
    shuttle_raw = (aggregated_raw >> 6) & 0b111;
    
    INCREMENT(ballast, ballast_raw);
    INCREMENT(spool, spool_raw);
    INCREMENT(shuttle, shuttle_raw);
}

void pwm_update(uint8_t num, uint16_t val) {
    val = (val < 4095) ? val : 4096;

    //Write a whole bunch of magic
    //Refer to the PCA9685 datasheet for more information
    Wire.beginTransmission(PWM_ADDR);
    uint8_t data[5] = {0x06 + 4*num, 0, 0, val, val>>8};
    Wire.write(data, 5);
    Wire.endTransmission();
}

// Emergency abort routine
// (just shuts down the Mega and lets the backup controller do its thing)
void ABORT() {
    for(;;);
}

void setup() {
    Serial.begin(BAUD_RATE);     //USB
    Serial1.begin(BAUD_RATE);    //Radio/mini-sub

    pid[0] = { .p = 0.1, .i = 0, .d = 0 }; //Ballast PID
    pid[1] = { .p = 0.1, .i = 0, .d = 0 }; //Spool PID

    //Register config
    DDRA &= 0b00000000;
    DDRC &= 0b00000000;
    DDRG &= 0b00000111;
    DDRL &= 0b00000000;
}

uint32_t old_mils;

void loop() {
    old_mils = millis();

    byte checksum = 0;

    //Recieve packet from groundstation
    Serial.readBytes(in_pack_buf, IN_PACKET_SIZE);

    //Confirm packet integrity
    for(int i = 0; i < IN_PACKET_SIZE; i++) {
        checksum ^= in_pack_buf[i];
    }

    if(!checksum) {
        shaft_voltage = in_pack_buf[0];
        ballast_set = *(uint16_t *) (in_pack_buf + 4);
        spool_set = *(uint16_t *) (in_pack_buf + 6);
        fore_plane_set = in_pack_buf[1] << 6;
        aft_plane_set = in_pack_buf[2] << 6;
        rudder_set = in_pack_buf[3] << 6;
    }
    
    // TODO: Redo flood determination code
    //Read flooding data and act accordingly
    flooded = PORTG;
    if(flooded) ABORT();
    
    read_encoders();
    
    pwm_update(0, shaft_voltage << 8);
    pwm_update(1, update_pid(pid[0], ballast_set, ballast_pos));
    pwm_update(2, fore_plane_set); //Fore dive planes
    pwm_update(3, aft_plane_set); //Aft dive planes
    pwm_update(4, rudder_set); //Aft rudder
    pwm_update(5, update_pid(pid[1], spool_set, spool_pos));

    // TODO: Figure out what the heck we're doing with the running lights


    byte out_pack_buf[OUT_PACKET_SIZE];

    //Populate packet with data
    *(float *) out_pack_buf = shaft_speed;
    *(uint16_t *) (out_pack_buf + 2) = ballast_pos;
    *(uint16_t *) (out_pack_buf + 4) = spool_pos;
    out_pack_buf[6] = flooded;
    out_pack_buf[7] = shaft_voltage;

    //Compute and append checksum
    checksum = 0;
    for(uint8_t i = 0; i < OUT_PACKET_SIZE - 1; i++) {
        checksum ^= out_pack_buf[i];
    }
    
    out_pack_buf[OUT_PACKET_SIZE - 1] = checksum;
    Serial.write(out_pack_buf, OUT_PACKET_SIZE);

    while(millis() - old_mils < D_MILS); {
        old_mils = millis();
    }
}
