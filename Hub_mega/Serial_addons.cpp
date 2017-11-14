#include "Serial_addons.h"

//Function for sending floats over serial as 2-wide byte arrays
void serialWrite(float val, byte bus){
  byte buf[2];
  memcpy(buf, &val, 2);
    
  switch(bus) {
    case 1:
      Serial1.write(buf, 2);
      break;
    case 2:
      Serial2.write(buf, 2);
      break;
    case 3:
      Serial3.write(buf, 2);
      break;
    default:
      Serial.write(buf, 2);
  }
}

float serialReadFloat(byte bus){
  while(!Serial.available());
  byte buf[2];                     
  switch(bus) {
    case 1:
      Serial1.readBytes(buf, 2);
      break;
    case 2:
      Serial2.readBytes(buf, 2);
      break;
    case 3:
      Serial3.readBytes(buf, 2);
      break;
    default:
      Serial.readBytes(buf, 2);
  }
  return *((float*)&buf);
}

//At least this one's easy
byte serialReadByte(byte bus){
  while(!Serial.available());
  switch(bus) {
    case 1:
      return Serial1.read();
    case 2:
      return Serial2.read();
    case 3:
      return Serial3.read();
    default:
      return Serial.read();
  }
}

