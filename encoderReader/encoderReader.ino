/*
 * C Register on mega for pins 30-37, D register on 328 for 0-7
 */

//stores the tick count
volatile int tickCount;
//pin that the phototransistor circuit is connected to
const int photoPin = 3;
void setup() {
  //set the C HW register to all output (1). Change to D for 328 equipped arduino
  DDRC = B11111111;
 /* pinMode(30, OUTPUT);
  pinMode(31, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(34, OUTPUT);
  pinMode(35, OUTPUT);
  pinMode(36, OUTPUT);
  pinMode(37, OUTPUT);*/
  tickCount = 0;
  //create an interrupt routine using a DI pin, ISR, and a positive edge trigger
  attachInterrupt(digitalPinToInterrupt(photoPin), updateCount, RISING);

}
void loop() {
}
/**
 * Updates the tick counter when called, then uses writeToPins() to update the 8 bit digital interface
 * This also functions as an ISR
 */
void updateCount(){
  /* Update this logic if using a bi-directional encoder system
  if(POSITIVE_EDGE_TRIGGER_CONDITIONAL){
    tickCount++;
  }
  else{
    tickCount--;
  }
  */
  tickCount++;
  //prevent 8 bit rollover
  if(tickCount == 256){
    tickCount = 0;
  }
  /* update this if using a bi-directional encoder system
  else if(tickCount == -32678){
    tickCount == 255;
  }
  */
  writeToPins();
}
/**
 * writes count to pins 30-37 on mega
 */
void writeToPins(){
  
  byte pinsByte = byte(tickCount);
  PORTC = pinsByte;
  
 /* int tempCount = tickCount;
  for(int i = 0; i < 8; i++){
    if(tempCount % 2 == 1){
      digitalWrite(i+30, HIGH);
    }
    else{
      digitalWrite(i+30, LOW);
    }
    tempCount = tempCount / 2;
  }*/
}
