#ifndef SERIAL_ADDONS_H
#define SERIAL_ADDONS_H

#include <stdlib.h>
#include <arduino.h>

//This is a small collection of methods to simplify serial communication, especially regarding floating point numbers

//Bus is initialized to 0 if not provided
void serialWrite(float val, byte bus = 0);
float serialReadFloat(byte bus = 0);
byte serialReadByte(byte bus = 0);

#endif
