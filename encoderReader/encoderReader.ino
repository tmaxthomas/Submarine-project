/*
 * Data is sent over DIO pins 4-11, using the 4 high bytes of register D and the 4 low bytes of register B
 */
 
//stores the tick count
volatile byte tick_count;

//pin that the phototransistor circuit is connected to
const int photoPin = 3;

void setup() {
  //Configure output pins
  DDRD |= 0b11110000;
  DDRB |= 0b1111;
  tick_count = 0;
  //create an interrupt routine using a DI pin, ISR, and a positive edge trigger
  attachInterrupt(digitalPinToInterrupt(photoPin), updateCount, RISING);
  //Prevent loop() from ever being called, because repeated calls to loop slow things down
  for(;;);
}

void loop() {}

/**
 * Updates the tick counter when called, then writes the updated tick count to the pins
 * Counter should automatically wrap around to 0
 */
void updateCount(){
  tick_count++;
  PORTD = tick_count << 4;
  PORTB = tick_count >> 4;
}
