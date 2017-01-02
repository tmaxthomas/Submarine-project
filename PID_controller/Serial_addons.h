#ifndef SERIAL_ADDONS_H
#define SERIAL_ADDONS_H

//This is a small collection of methods to simplify serial communication, especially regarding floating point numbers

//For arduinos other than the Mega
void serialWrite(float val);
float serialReadFloat();
byte serialReadByte();

//For the Mega and its multiple serial buses
void serialWrite(float val, int bus);
float serialReadFloat(int bus);
byte serialReadByte(int bus);

#endif
