#include <stdint.h>

// Stores the tick counts
volatile uint16_t aggregated_counter;

inline static void update_ports() {
    PORTD = (uint8_t) (aggregated_counter << 4);
    PORTB = (uint8_t) (aggregated_counter >> 4);
}

void update_ballast(){
    uint16_t tick_count = ((aggregated_counter & 0b111) + 1) & 0b111;
    aggregated_counter = (aggregated_counter & 0b111111000) | tick_count;
    update_ports();
}

// Spool
void update_spool(){
    uint16_t tick_count = ((aggregated_counter & 0b111000) + 0b1000) & 0b111000;
    aggregated_counter = (aggregated_counter & 0b111000111) | tick_count;
    update_ports();
}

// Shuttle
void update_shuttle(){
    uint16_t tick_count = ((aggregated_counter & 0b111000000) + 0b1000000) & 0b111000000;
    aggregated_counter = (aggregated_counter & 0b111111) | tick_count;
    update_ports();
}

void setup() {
    // Configure output pins
    DDRD |= 0b11110000;
    DDRB |= 0b11111;
    aggregated_counter = 0;

    attachInterrupt(digitalPinToInterrupt(2), update_ballast, RISING);
    attachInterrupt(digitalPinToInterrupt(3), update_spool, RISING);
    attachInterrupt(digitalPinToInterrupt(4), update_shuttle, RISING);
}
