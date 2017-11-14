#include "PID.h"

//Recursive PID update function

//The sent byte indicates which, if any, new values for setpoint, p, i, and d have been sent, saving us the trouble
//of going through the messy process of reading four additional floating-point values off the serial line every single clock cycle
//for every single PID controller

//Of course, the capacity to change P, I, and D vals at any point can be removed if need be (and, to be honest, it probably will be)

void PID::update() {
  float input = serialReadFloat();
  byte sent;
  sent = serialReadByte();
  if(sent & 0b00000001) set_pt = serialReadByte();
  if(sent & 0b00000010) p = serialReadFloat();
  if(sent & 0b00000100) i = serialReadFloat();
  if(sent & 0b00001000) d = serialReadFloat();
  err = set_pt - input;
  total_err += err;
  d_input = old_input - input;
  old_input = input;
  serialWrite(err * p + total_err * i - d_input * d); 
  if(next) next->update();
}

