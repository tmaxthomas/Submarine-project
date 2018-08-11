#include <stdint.h>

// Stores the tick counts
volatile uint16_t aggregated_counter;

void update_ports() {
    PORTD = (uint8_t) (aggregated_counter << 4);
    PORTB = (uint8_t) (aggregated_counter >> 4);

}

void update_count_2(){
    uint8_t tick_count = (aggregated_counter & 0b111) + 1;
    aggregated_counter = (aggregated_counter & 0b111111000) | tick_count;
    update_ports();
}

void update_count_3(){
    uint8_t tick_count = (aggregated_counter & 0b111000) + 0b1000;
    aggregated_counter = (aggregated_counter & 0b111000111) | tick_count;
    update_ports();
}


void update_count_4(){
    uint8_t tick_count = (aggregated_counter & 0b111000000) + 0b1000000;
    aggregated_counter = (aggregated_counter & 0b111111) | tick_count;
    update_ports();
}

void setup() {
    // Configure output pins
    DDRD |= 0b11110000;
    DDRB |= 0b11111;
    aggregated_counter = 0;

    attachInterrupt(digitalPinToInterrupt(2), update_count_2, RISING);
    attachInterrupt(digitalPinToInterrupt(3), updateCount, RISING);
    attachInterrupt(digitalPinToInterrupt(4), updateCount, RISING);
  
    // Prevent loop() from ever being called, because repeated calls to loop slow things down
    for(;;);
}

void loop() {}
