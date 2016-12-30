//A few notes on how this deivates from bog-standard PID:

//We don't ever consider the time between actions because it should be constant-that sort of thing is
//controlled by the frequency of new error values being sent over serial from the central Mega

//We use -dInput instead of dError to avoid problems from what are called derivative spikes
//Derivative spikes are caused by sudden changes in the setpoint, resulting in big changes
//to dError. dInput, it turns out, is always equal to -dError, except when the setpoint changes.
//Just swap -dInput for dError, and derivative spikes go away

//Oh, and the values of the setpoint, p, i, and d can be changed at any point without actually having to send
//them every cycle on the Mega, at the cost of one byte per transmission
//The sent byte is used to indicate which updated values, if any, have been sent
//This is more efficient than sending the values every cycle-assuming it takes longer to send something
//over serial than it takes to do binary arithmetic

class PID{
public:
  PID(float p_ = 0, float i_ = 0, float d_ = 0) : p(p_), i(i_), d(d_), set_pt(0), old_input(0), total_err(0) {}
  void update();
  float p, i, d;
  float set_pt, input, old_input, err, total_err, d_input;
  PID* next;
};

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
void PID::update() {
  float input = serialRead();
  byte sent = serialRead();
  if(sent & 0b00000001 != 0) set_pt = serialRead();
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

PID* head = NULL;

void setup() {
  Serial.begin(9600);
  bool reading = true;
  PID* curr;
  while(reading){
    float input = serialRead();
    float p, i, d;
    if(input != 0){
      p = input;
      i = serialRead();
      d = serialRead();
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

