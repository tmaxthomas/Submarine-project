#include <stdint.h>
#include <Wire.h>

#define D_MILS 20 //Length of cycle, in milliseconds
#define BAUD_RATE 115200 //Baud rate (duh)

//Macro for reading encoder data off of registers
#define ReadReg(Comp, Reg) uint8_t Comp ## _new = Reg; \
                           delta = Comp ## _new - Comp ## _old; \
                           Comp ## _old = Comp ## _new; \
                           Comp ## _pos += ( Comp ## _dir ? delta : -delta )

#define PWMWrite(addr, val) Wire.beginTransmission(PWM_ADDR); \
                            Wire.write(addr); \
                            Wire.write(val); \
                            Wire.endTransmission()


//Sizes for outgoing and incoming packets. Subject to change once we actually figure out what we're sending.
#define OUT_PACKET_SIZE 9
#define IN_PACKET_SIZE 9

// I2C address of the PWM controller
#define PWM_ADDR 0x40

uint8_t in_pack_buf[IN_PACKET_SIZE], out_pack_buf[OUT_PACKET_SIZE];

//Encoder data
uint16_t spool_pos, shaft_pos, ballast_pos;
uint16_t spool_set, ballast_set;
uint8_t spool_old, shaft_old, ballast_old;
bool spool_dir, ballast_dir, shaft_dir;
float shaft_speed;

//Other sensor/misc data
uint8_t flooded;
int8_t shaft_voltage;
uint16_t fore_plane_set, aft_plane_set, rudder_set;

//PID struct
struct PID {
    float p, i, d;
    float total_err, old_val;
} ctrl[2];

//Updates PID controller and returns updated motor power percentage
//NOTE: Loops need to be tuned to produce values in the range 0 : 16,383
uint16_t updatePID(struct PID c, uint16_t set_pt, uint16_t val) {
    float err = set_pt - val;
    c.total_err += err;
    float d_val = c.old_val - val;
    c.old_val = val;
    return (err * c.p) + (c.total_err * c.i) - (d_val * c.d);
}

// Runs initialization code for the PWM controller
void pwm_controller_init() {
    //Put the controller to sleep in order to prep for prescale
    PWMWrite(0x00, 0x10);

    //Write prescale
    PWMWrite(0xFE, 0x79);

    //Wake the controller back up and turn auto-increment on
    PWMWrite(0x00, 0x20);
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

uint32_t old_mils;

void setup() {
    Serial.begin(BAUD_RATE);     //USB
    Serial1.begin(BAUD_RATE);    //Radio/mini-sub

    ctrl[0] = { .p = 0.1, .i = 0, .d = 0 }; //Ballast PID
    ctrl[1] = { .p = 0.1, .i = 0, .d = 0 }; //Spool PID

    //Register config
    DDRA &= 0b00000000;
    DDRC &= 0b00000000;
    DDRG &= 0b00000111;
    DDRL &= 0b00000000;

    old_mils = millis();
}

void loop() {
    byte checksum = 0;

    //Recieve packet from groundstation
    Serial.readBytes(in_pack_buf, IN_PACKET_SIZE);

    //Confirm packet integrity
    for(int i = 0; i < IN_PACKET_SIZE; i++)
        checksum ^= in_pack_buf[i];

    if(!checksum) {
        shaft_voltage = in_pack_buf[0];
        ballast_set = *(uint16_t *) (in_pack_buf + 4);
        spool_set = *(uint16_t *) (in_pack_buf + 6);
        fore_plane_set = in_pack_buf[1] << 6;
        aft_plane_set = in_pack_buf[2] << 6;
        rudder_set = in_pack_buf[3] << 6;
    }
    
    //TODO: Redo flood determination code
    //Read flooding data and act accordingly
    flooded = PORTG;
    if(flooded) ABORT();

    //Reading from encoder Arduinos
    byte delta;
    //Pins 37 (PORTC 0) to 30 (PORTC 7) - Remember to plug this one in backwards
    ReadReg(ballast, PORTC);

    //Pins 22 (PORTA 0) to 29 (PORTA 7)
    ReadReg(spool, PORTA);

	//Pins 49 (PORTL 0) to 42 (PORTL 7) - This one goes in backwards as well
    ReadReg(shaft, PORTL);


    pwm_update(0, shaft_voltage << 8);
    pwm_update(1, updatePID(ctrl[0], ballast_set, ballast_pos));
    pwm_update(2, fore_plane_set); //Fore dive planes
    pwm_update(3, aft_plane_set); //Aft dive planes
    pwm_update(4, rudder_set); //Aft rudder
    pwm_update(5, updatePID(ctrl[1], spool_set, spool_pos));

    //TODO: Figure out what the heck we're doing with the running lights


    byte out_pack_buf[OUT_PACKET_SIZE];

    //Populate packet with data
    *(float *) out_pack_buf = shaft_speed;
    *(uint16_t *) (out_pack_buf + 2) = ballast_pos;
    *(uint16_t *) (out_pack_buf + 4) = spool_pos;
    out_pack_buf[6] = flooded;

    //Compute and append checksum
    checksum = 0;
    for(uint8_t i = 0; i < OUT_PACKET_SIZE - 2; i++)
        checksum ^= out_pack_buf[i];
    out_pack_buf[OUT_PACKET_SIZE - 2] = checksum;

    //Send main sub shaft voltage to mini-sub
    //TODO: determine conversion formula from main shaft voltage to mini-sub shaft voltage
    out_pack_buf[OUT_PACKET_SIZE - 1] = shaft_voltage;

    Serial.write(out_pack_buf, OUT_PACKET_SIZE);

    while(millis() - old_mils < D_MILS);
        old_mils = millis();
}
