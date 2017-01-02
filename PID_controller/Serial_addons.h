#ifndef SERIAL_ADDONS_H
#define SERIAL_ADDONS_H

//This is a small collection of methods to simplify serial communication, especially regarding floating point numbers

void serialWrite(float val);
float serialReadFloat();
byte serialReadByte();

#endif
