#include <stdint.h>

// Stores the tick counts
volatile uint8_t tick_count_1;

/*
 * Updates the tick counter when called, then writes the updated tick count to the pins
 * Counter should automatically wrap around to 0
 */
void updateCount(){
    tick_count++;
    PORTD = tick_count << 4;
    PORTB = tick_count >> 4;
}


//pin that the phototransistor circuit is connected to
const int photoPin = 3;

void setup() {
    // Configure output pins
    DDRD |= 0b11110000;
    DDRB |= 0b1111;
    tick_count = 0;
    // Create an interrupt routine using a DI pin, ISR, and a positive edge trigger
    attachInterrupt(digitalPinToInterrupt(photoPin), updateCount, RISING);
  
    // Prevent loop() from ever being called, because repeated calls to loop slow things down
    for(;;);
}

void loop() {}


