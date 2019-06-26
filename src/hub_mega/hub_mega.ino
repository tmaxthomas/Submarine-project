/*
Submarine Arduino Mega Hub Code

Handles the transmission and reception of Serial data packets.
Writes all PWM data to the PWM Driver Board.
Reads the encoder bus.
Reads all analog sensors:
	-Water sensors
	-Temperature sensors
	-Potentiometer Feedback Sensors
Runs the control algorithm for interpreting and repositioning mechanical systems

*/

/*
Program Flow:

Declare variables, include libraries, define constants

Setup:
	-initialize Serial1
	-initialize PWM driver board
	-Initialize Radio
	-set pinmodes, where necessary
	
Loop:
	-Start by checking the radio for a new packet. If present, update local setpoints with new packet data
	-begin read operations:
		-Read encoder bus, increment or decriment as necessary
		-Read Potentiometers
		-Read Temperature Sensors
		-Read water sensor
		
	-begin write operations
		-send servo writes, if new setpoints have been assigned
		-Send new headlight value, if assigned
		-Send ballast ESC write with PI control
		-Send Spool/Carriage writes:
			-Start by checking if setpoint != current spool encoder count
			-If it is different, then write appropriate direction to spool servo (don't move otherwise)
				-check current spool encoder count against carriage encoder count (with scaling factor for spool counts:carriage counts per rev of spool)
					(use iterating factor - number of times the carriage has made a full travel)
					Determine where the carriage needs to be (from spool count) vs where it is. The spool count acts as the carriage setpoint
						Assign appropriate write, if necessary 
	
	-assemble new transmit data packet
		-write new data packet

*/
#include <Wire.h>
#include "../common.h"
#include <Adafruit_PWMServoDriver.h>

/******************
 * MACROS/DEFINES *
 ******************/

//Length of cycle, in microseconds (min = 10, max  = 15000)
#define D_MILS 10000

/*************
* PWM LIMITS *
*************/

//Carriage. PWM below Center sends carriage towards aft. 
#define CARRIAGE_MIN		330
#define CARRIAGE_CENTER 	363
#define CARRIAGE_MAX		396

//Spool. PWM below Center Spools out the tether
#define SPOOL_MIN 			340
#define SPOOL_CENTER 		374
#define SPOOL_MAX 			410

//Rudder Servo. PWM below Center Steers sub to right
#define RUDDER_MIN 			340 
#define RUDDER_CENTER 		395
#define RUDDER_MAX 			470

//Aft Dive Servo. PWM below Center points planes downwards as if to Dive.
#define AFT_DIVE_MIN 		260
#define AFT_DIVE_CENTER 	320
#define AFT_DIVE_MAX 		380

//Fore Dive Servo. PWM below Center points planes downwards as if to surface.
#define FORE_DIVE_MIN 		260
#define FORE_DIVE_CENTER 	355
#define FORE_DIVE_MAX 		425

//Headlights. 0 is off, 4096 max on
#define HEADLIGHT_MIN 		0
#define HEADLIGHT_MAX 		4096

//Drive Motor ESC. TODO: check directions
#define DRIVE_MIN 			300
#define DRIVE_CENTER 		350
#define DRIVE_MAX 			400

//Ballast Motor ESC. PWM below center pushes water out of ballast. 
#define BALLAST_MIN 		300
#define BALLAST_CENTER 		350
#define BALLAST_MAX 		400



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

//NRF Radio limited to 32bytes in the read buffer. plan accordingly. 
struct in_pack_t in_packet;
struct out_pack_t out_packet;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

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

// Reads and records updated encoder values off of the parallel busses coming from
// the encoder Nanos
inline void read_encoders() {
    
    uint8_t ballast_raw, spool_raw, shuttle_raw;
    uint16_t aggregated_raw = ((PORTC & 0b11111000) << 1) + (PORTL & 0b1111);
    ballast_raw = aggregated_raw & 0b111;
    spool_raw = (aggregated_raw >> 3) & 0b111;
    shuttle_raw = (aggregated_raw >> 6) & 0b111;

    increment(&ballast, ballast_raw);
    increment(&spool, spool_raw);
    increment(&shuttle, shuttle_raw);
}


// Emergency abort routine
void emerg_abort() {
    for(;;);
}

/**************
 * SETUP/LOOP *
 **************/

void setup() {
    Serial.begin(BAUD_RATE);     //USB
    Serial1.begin(BAUD_RATE);    //Radio/mini-sub
	
	//Initialize the PWM Driver Board
	pwm.begin();
	pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
	
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
