#include "PID.h"

//Yes, really.  Serial can only transmit byte packages (or arrays of them). 
//Floats are 4 bytes long. Therefore, we can send them as arrays of 4 bytes
//However... we have to go through a void pointer first, because we're doing
//an extremely non-standard type conversion.
void serialWrite(float val){
  void* ptr = &val;
  byte buf[4] = { 0 };
  byte* byte_ptr = (byte*)ptr;
  for(int a = 0; a < 4; a++){
    buf[a] = *byte_ptr;
    byte_ptr++;
  }
  Serial.write(buf, 4);
}

float serialRead(){
  while(!Serial.available()); //Wait until something comes along the serial bus
  byte buf[4];                //Then screw around with void pointers and typecasting      
  Serial.readBytes(buf, 4);
  void* ptr = buf;
  float* float_ptr = static_cast<float*>(ptr);
  return *float_ptr;
}

//At least this one's easy
void serialRead(byte& val){
  while(!Serial.available());
  val = Serial.read();
}

//Recursive PID update function
//The sent byte indicates which, if any, new values for setpoint, p, i, and d have been sent, saving us the trouble
//of going through the messy process of reading four additional floating-point values off the serial line every single clock cycle
//for every single PID controller
void PID::update() {
  float input = serialRead();
  byte sent;
  serialRead(sent);
  if(sent & 0b00000001 != 0) serialRead(set_pt);
  if(sent & 0b00000010 != 0) p = serialRead();
  if(sent & 0b00000100 != 0) i = serialRead();
  if(sent & 0b00001000 != 0) d = serialRead();
  err = set_pt - input;
  total_err += err;
  d_input = old_input - input;
  old_input = input;
  serialWrite(err * p + total_err * i - d_input * d); 
  if(next) next->update();
}
