#include "PID.h"
#include "Serial_addons.h"

//A few notes on how this deivates from bog-standard PID:

//We don't ever consider the time between actions because it should be constant-that sort of thing is
//controlled by the frequency of new error values being sent over serial from the central Mega

//We use -dInput instead of dError to avoid problems from what are called derivative spikes
//Derivative spikes are caused by sudden changes in the setpoint, resulting in big changes
//to dError. dInput, it turns out, is always equal to -dError, except when the setpoint changes.
//Just swap -dInput for dError, and derivative spikes go away

//Oh, and the values of the setpoint, p, i, and d can be changed at any point without actually having to send
//them every cycle on the Mega, at the cost of one byte per transmission

PID* head = NULL; //Keeps track of the head of the list of PID controllers

//Creates all the PID controllers from data sent from the Mega. The idea here is that we only have to edit the Mega's code to change PID values-
//we program this, get it working, and then treat it like a black box. It also means a start-up period for the system, but eh.
void setup() {
  Serial.begin(9600);
  bool reading = true;
  PID* curr;
  while(reading){
    float input = serialReadFloat();
    float p, i, d;
    if(input != 0){
      p = input;
      i = serialReadFloat();
      d = serialReadFloat();
      if(!head){
        head = new PID(p, i, d);
        curr = head;
      } else {
        curr->next = new PID(p, i, d);
        curr = curr->next;
      }
    } else {
      reading = false;
      curr->next = NULL;
    }
  }
}

void loop() {
  head->update();
}
