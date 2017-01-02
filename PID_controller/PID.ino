#include "PID.h"
#include "Serial_addons.h"

//Recursive PID update function
//The sent byte indicates which, if any, new values for setpoint, p, i, and d have been sent, saving us the trouble
//of going through the messy process of reading four additional floating-point values off the serial line every single clock cycle
//for every single PID controller
void PID::update() {
  float input = serialReadFloat();
  byte sent;
  sent = serialReadByte();
  if(sent & 0b00000001 != 0) set_pt = serialReadByte();
  if(sent & 0b00000010 != 0) p = serialReadFloat();
  if(sent & 0b00000100 != 0) i = serialReadFloat();
  if(sent & 0b00001000 != 0) d = serialReadFloat();
  err = set_pt - input;
  total_err += err;
  d_input = old_input - input;
  old_input = input;
  serialWrite(err * p + total_err * i - d_input * d); 
  if(next) next->update();
}
