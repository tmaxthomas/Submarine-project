#define D_MILS 20 //Length of cycle, in milliseconds
#define BAUD_RATE 115200 //Baud rate (duh)

//Macro for reading encoder data off of registers
#define ReadReg(Comp, Reg) byte Comp ## _new = Reg; \
                           delta = Comp ## _new - Comp ## _old; \
                           Comp ## _old = Comp ## _new; \
                           Comp ## _pos += ( Comp ## _dir ? delta : -delta )


//Sizes for outgoing and incoming packets. Subject to change once we actually figure out what we're sending.
#define OUT_PACKET_SIZE 32
#define IN_PACKET_SIZE 32

//Encoder data
int spool_pos, shaft_pos, ballast_pos;
byte spool_old, shaft_old, ballast_old;
bool spool_dir, ballast_dir, shaft_dir;
float shaft_speed;

//Other sensor data

//PID struct
struct PID {
  float p, i, d;
  float total_err, old_val;
};

//Updates PID controller and returns updated motor power percentage
float updatePID(struct PID ctrl, float set_pt, float val) {
  float err = set_pt - val;
  ctrl.total_err += err;
  float d_val = ctrl.old_val - val;
  ctrl.old_val = val;
  return (err * ctrl.p) + (ctrl.total_err * ctrl.i) - (d_val * ctrl.d);
}

int old_mils;
void setup() {
  Serial.begin(BAUD_RATE);     //USB
  Serial1.begin(BAUD_RATE);    //Backup?
  Serial2.begin(BAUD_RATE);    //Radio/mini-sub

  struct PID ctrl[2];
  ctrl[0] = { .p = 0.01, .i = 0, .d = 0 };
  ctrl[1] = { .p = 0.01, .i = 0, .d = 0 };

  //Register config
  DDRC = 0b00000000;
  DDRA = 0b00000000;
  DDRL = 0b00000000;

  old_mils = millis();
}

void loop() {
  //May want to reconsider how we pass data, accounting for how damn long it takes

  byte checksum = 0;
  //Recieve packet from groundstation
  byte in_packet[IN_PACKET_SIZE];
  Serial.readBytes(in_packet, IN_PACKET_SIZE);

  //Confirm packet integrity
  for(int i = 0; i < IN_PACKET_SIZE; i++)
    checksum ^= in_packet[i];

  if(!checksum) {
    /*
     * TODO: RETRIEVE DATA FROM PACKET
     */
  }

  //TODO: Read from sensors

  //Reading from encoder Arduinos
  byte delta;
  //Pins 37 (PORTC 0) to 30 (PORTC 7) - Remember to plug this one in backwards
  ReadReg(ballast, PORTC);

  //Pins 22 (PORTA 0) to 29 (PORTA 7)
  ReadReg(spool, PORTA);

  //Pins 49 (PORTL 0) to 42 (PORTL 7) - This one goes in backwards as well
  ReadReg(shaft, PORTL);

  //TODO: Update PID controllers and write new vals to motor controllers

  //TODO: Send data packet to groundstation/minisub
  byte out_packet[OUT_PACKET_SIZE];
  /*
   * TODO: POPULATE PACKET
   */

  //Compute and append checksum
  checksum = 0;
  for(int i = 0; i < OUT_PACKET_SIZE - 1; i++)
    checksum ^= out_packet[i];
  out_packet[OUT_PACKET_SIZE - 1] = checksum;

  Serial.write(out_packet, OUT_PACKET_SIZE);

  while(millis() - old_mils < D_MILS);
  old_mils = millis();
}
