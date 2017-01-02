#include "Serial_addons.h"

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

float serialReadFloat(){
  while(!Serial.available()); //Wait until something comes along the serial bus
  byte buf[4];                //Then screw around with void pointers and typecasting      
  Serial.readBytes(buf, 4);
  void* ptr = buf;
  float* float_ptr = (float*)ptr;
  return *float_ptr;
}

//At least this one's easy
byte serialReadByte(){
  while(!Serial.available());
  return Serial.read();
}
