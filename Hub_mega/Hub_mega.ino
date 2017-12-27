#include <stdint.h>
#include <Wire.h>

#define D_MILS 20 //Length of cycle, in milliseconds
#define BAUD_RATE 115200 //Baud rate (duh)

//Macro for reading encoder data off of registers
#define ReadReg(Comp, Reg) uint8_t Comp ## _new = Reg; \
                           delta = Comp ## _new - Comp ## _old; \
                           Comp ## _old = Comp ## _new; \
                           Comp ## _pos += ( Comp ## _dir ? delta : -delta )


//Sizes for outgoing and incoming packets. Subject to change once we actually figure out what we're sending.
#define OUT_PACKET_SIZE 9
#define IN_PACKET_SIZE 9

#define PWM_ADDR 0x40

uint8_t in_pack_buf[IN_PACKET_SIZE], out_pack_buf[OUT_PACKET_SIZE];

//Encoder data
uint16_t spool_pos, shaft_pos, ballast_pos;
uint8_t spool_old, shaft_old, ballast_old;
bool spool_dir, ballast_dir, shaft_dir;
float shaft_speed;


//Other sensor data

//PID struct
struct PID {
  float p, i, d;
  float total_err, old_val;
} ctrl[2];



//Updates PID controller and returns updated motor power percentage
float updatePID(struct PID c, float set_pt, float val) {
  float err = set_pt - val;
  c.total_err += err;
  float d_val = c.old_val - val;
  c.old_val = val;
  return (err * c.p) + (c.total_err * c.i) - (d_val * c.d);
}

// Runs initialization code for the PWM controller
void pwm_controller_init() {
    Wire.beginTransmission(PWM_ADDR);
    Wire.write(0x0);
    Wire.write(0x30);
}

void pwm_update(uint8_t num, uint16_t val) {
    //I really don't trust Arduino's built-in min function
    val = (val < 4096) ? val : 4096;

    //Write a whole bunch of magic
    //Refer to the PCA9685 datasheet for more information
    Wire.beginTransmission(PWM_ADDR);
    Wire.write(0x6 + 4*num);
    Wire.write(0);
    Wire.write(0);
    Wire.write(val);
    Wire.write(val>>8);
    Wire.endTransmission();
}

uint32_t old_mils;

void setup() {
  Serial.begin(BAUD_RATE);     //USB
  Serial1.begin(BAUD_RATE);    //Radio/mini-sub

  ctrl[0] = { .p = 0.01, .i = 0, .d = 0 };
  ctrl[1] = { .p = 0.01, .i = 0, .d = 0 };

  //Register config
  DDRC = 0b00000000;
  DDRA = 0b00000000;
  DDRL = 0b00000000;

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
    /*
     * TODO: RETRIEVE DATA FROM PACKET
     */
  }

  //TODO: Read from sensors

  //Reading from encoder Arduinos
  byte delta;
  //Pins 37 (PORTC 0) to 30 (PORTC 7) - Remember to plug this one in backwards
  ReadReg(ballast, PORTC);

  //Pins 22 (PORTA 0) to 29 (PORTA 7)
  ReadReg(spool, PORTA);

  //Pins 49 (PORTL 0) to 42 (PORTL 7) - This one goes in backwards as well
  ReadReg(shaft, PORTL);

  //TODO: Update PID controllers and write new vals to motor controllers

  //TODO: Send data packet to groundstation/minisub
  byte out_packet[OUT_PACKET_SIZE];
  /*
   * TODO: POPULATE PACKET
   */

  //Compute and append checksum
  checksum = 0;
  for(uint8_t i = 0; i < OUT_PACKET_SIZE - 1; i++)
    checksum ^= out_pack_buf[i];

  out_pack_buf[OUT_PACKET_SIZE - 1] = checksum;

  Serial.write(out_pack_buf, OUT_PACKET_SIZE);

  while(millis() - old_mils < D_MILS);
  old_mils = millis();
}
