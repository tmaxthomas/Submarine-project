/*
 * Data is sent over DIO pins 4-11, using the 4 high bytes of register D and the 4 low bytes of register B
 */
//stores the tick count
volatile unsigned byte tick_count;

//pin that the phototransistor circuit is connected to
const int photoPin = 3;

void setup() {
  //Configure output pins
  DDRD |= 0b11110000;
  DDRB |= 0b1111;
  tick_count = 0;
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
  tick_count++;
  writeToPins();
}
/**
 * writes count to pins 4-11 on Nano through the magic of bitwise arithmetic
 */
 
void writeToPins(){
  PORTD = tick_count << 4;
  PORTB = tick_count >> 4;
}
