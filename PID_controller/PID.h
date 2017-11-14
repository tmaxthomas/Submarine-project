#ifndef PID_H_
#define PID_H_

#include <arduino.h>
#include "Serial_addons.h"

class PID{
public:
  PID(float p_ = 0, float i_ = 0, float d_ = 0) : p(p_), i(i_), d(d_), set_pt(0), old_input(0), total_err(0) {}
  void update();
  float p, i, d;
  byte set_pt;
  float input, old_input, err, total_err, d_input;
  PID* next;
};

#endif
