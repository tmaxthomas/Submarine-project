#include <stdint.h>

/* Data is sent over DIO pins 4-11, using the 4 high bytes of register D and
 * the 4 low bytes of register B
 */

// Stores the tick count
volatile uint8_t tick_count;

/* Updates the tick counter when called, then writes the updated tick count to the pins
 * Counter should automatically wrap around to 0
 */
void updateCount(){
    tick_count++;
    PORTD = tick_count << 4;
    PORTB = tick_count >> 4;
}

void setup() {
    // Configure output pins
    DDRD |= 0b11110000;
    DDRB |= 0b1111;
    tick_count = 0;
    // Create an interrupt routine using a DI pin, ISR, and a positive edge trigger
    attachInterrupt(digitalPinToInterrupt(3), updateCount, RISING);
}

void loop() {}
