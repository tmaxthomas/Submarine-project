/*
 * Data is sent over DIO pins 4-11, using the 4 high bytes of register D and the 4 low bytes of register B
 */
//stores the tick count
volatile unsigned byte tickCount;

//pin that the phototransistor circuit is connected to
const int photoPin = 3;

void setup() {
  //Configure output pins
  DDRD |= 0b11110000;
  DDRB |= 0b1111;
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
  writeToPins();
}
/**
 * writes count to pins 4-11 on Nano
 */
 
void writeToPins(){
  byte d_byte = tickCount << 4, b_byte = tickCount >> 4;
  PORTD = d_byte;
  PORTB = b_byte;
}
