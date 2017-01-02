#include "Serial_addons.h"

//Yes, really.  Serial can only transmit byte packages (or arrays of them). 
//Floats are 4 bytes long. Therefore, we can send them as arrays of 4 bytes
//However... we have to go through a void pointer first, because we're doing
//an extremely non-standard type conversion.
void serialWrite(float val, int bus){
  void* ptr = &val;
  byte buf[4] = { 0 };
  byte* byte_ptr = (byte*)ptr;
  for(int a = 0; a < 4; a++){
    buf[a] = *byte_ptr;
    byte_ptr++;
  }
  if(bus == 1)
    Serial1.write(buf, 4);
  else if(bus == 2)
    Serial2.write(buf, 4);
  else if(bus == 3)
    Serial3.write(buf, 4);
  else
    Serial.write(buf, 4);
}

void serialWrite(float val){
  serialWrite(val, 0);
}

float serialReadFloat(int bus){
  while(!Serial.available()); //Wait until something comes along the serial bus
  byte buf[4];                //Then screw around with void pointers and typecasting      
  if(bus == 1)
    Serial1.readBytes(buf, 4);
  else if(bus == 2)
    Serial2.readBytes(buf, 4);
  else if(bus == 3)
    Serial3.readBytes(buf, 4);
  else
    Serial.readBytes(buf, 4);
  void* ptr = buf;
  float* float_ptr = (float*)ptr;
  return *float_ptr;
}

float serialReadFloat(){
  return serialReadFloat(0);
}

//At least this one's easy
byte serialReadByte(int bus){
  while(!Serial.available());
  if(bus == 1)
    return Serial1.read();
  else if(bus == 2)
    return Serial2.read();
  else if(bus == 3)
    return Serial3.read();
  else
    return Serial.read();
}

byte serialReadByte() {
  return serialReadByte(0);
}

