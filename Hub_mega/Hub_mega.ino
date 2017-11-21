#include "Serial_addons.h"

#define NUM_PID 1 //Number of PID controllers
#define D_MILS 20 //Length of cycle, in milliseconds

//Macro for reading encoder data off of registers
#define ReadReg(Comp, Reg) byte Comp ## _new = Reg; \
                           delta = Comp ## _new - Comp ## _old; \
                           Comp ## _old = Comp ## _new; \
                           Comp ## _pos += delta


//Still need to implement register trickery

int spool_pos, shaft_pos, ballast_pos;
byte spool_old, shaft_old, ballast_old;
bool spool_dir, ballast_dir, shaft_dir;
float shaft_speed;

int old_mils;
void setup() {
  Serial.begin(115200);     //USB
  Serial1.begin(115200);    //PID controller
  Serial2.begin(115200);    //Radio/mini-sub
  Serial3.begin(115200);    
  //Put p, i, and d values for controllers in these arrays
  float p_vals[NUM_PID] = { 0.01 };
  float i_vals[NUM_PID] = { 0 };
  float d_vals[NUM_PID] = { 0 };
  //Then write the arrays (assuming writing to Serial1)
  for(int a = 0; a < NUM_PID; a++){
    serialWrite(p_vals[a], 1);
    serialWrite(i_vals[a], 1);
    serialWrite(d_vals[a], 1);
  }
  //Write signal-termination bytes
  serialWrite(0, 1);
  old_mils = millis(); 

  //Register config-set them all to output
  DDRC = 0b00000000;
  DDRA = 0b00000000;
  DDRL = 0b00000000;
}

void loop() {
  //Recieve packet from groundstation
  
  
  //Read from sensors (TODO)

  //Reading from encoder Arduinos
  byte delta;
  //Pins 37 (PORTC 0) to 30 (PORTC 7) - Remember to plug this one in backwards
  ReadReg(ballast, PORTC);

  //Pins 22 (PORTA 0) to 29 (PORTA 7) 
  ReadReg(spool, PORTA);

  //Pins 49 (PORTL 0) to 42 (PORTL 7) - This one goes in backwards as well
  ReadReg(shaft, PORTL);
  
  //Read/write from/to PID arduino (TODO)
  

  //Send data packet to groundstation/minisub
  byte packet[32];
  
  
  while(millis() - old_mils < D_MILS);
}
