#include "Serial_addons.h"

#define NUM_PID 1 //Number of PID controllers
#define D_MILS 20 //Length of cycle, in milliseconds

//Still need to implement register trickery

int spool_pos, shaft_pos, ballast_pos;
float shaft_speed;
unsigned byte spool_old, shaft_old, ballast_old;
byte delta;

int old_mils;
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  Serial2.begin(115200);
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

  //Register config
  DDRC = 0b0;
  DDRA = 0b0;
}

void loop() {
  //Read from sensors (TODO)

  //Reading from encoder Arduinos
  //Pins 37 (PORTC 0) to 30 (PORTC 7) - Remember to plug this one in backwards
  
  unsigned byte spool_new = PORTC;
  delta = spool_new - spool_old;
  spool_count += delta;

  //Pins 22 (PORTA 0) to 29 (PORTA 7) 

  unsigned byte ballast_new = PORTA;
  delta = ballast_new - ballast_old;
  ballast_count += delta;
  
  //Read/write from/to PID arduino (TODO)
  
  while(millis() - old_mils < D_MILS);
}
