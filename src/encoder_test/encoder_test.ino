/*
ARDUINO NANO ENCODER CODE
Reads the 3 Hall effect sensors and the 3 direction sense pins,
outputs the encoder increments on the 6 wire bus.

PINOUT:

Hall Effect Sensors
Carriage Hall Effect:	D11
Spool Hall Effect:		D13
Ballast Hall Effect:	D10

Direction Sense
Carriage Sense (Orange):	D9
Spool Sense (Purple):		D12
Ballast Sense (Blue):		D2

Parallel Bus
Carriage LSB (Blue):	D5
Carriage MSB (Purple):	D4
Spool LSB (Orange):		D7
Spool MSB (Yellow):		D8
Ballast LSB (Green):	D6
Ballast MSB (Grey):		D3

*/

//Establish Pin variables
uint8_t carriageHall =	11;
uint8_t spoolHall =		13;
uint8_t ballastHall =	10;

uint8_t carriageSense =	9;
uint8_t spoolSense =	12;
uint8_t ballastSense =	2;

uint8_t carriageLSB =	5;
uint8_t carriageMSB =	4;
uint8_t spoolLSB =		7;
uint8_t spoolMSB =		8;
uint8_t ballastLSB =	6;
uint8_t ballastMSB =	3;

//Debounce Counters
uint8_t carriageDebounce =	0;
uint8_t spoolDebounce =		0;
uint8_t ballastDebounce =	0;

uint8_t debounceThreshhold = 3;

uint16_t delayTime = 500;

void setup() {
	Serial.begin(9600);
	//Configure the GPIO as INPUT/OUTPUT
	pinMode(carriageHall, INPUT);
	pinMode(spoolHall, INPUT);
	pinMode(ballastHall, INPUT);
	
	pinMode(carriageSense, INPUT);
	pinMode(spoolSense, INPUT);
	pinMode(ballastSense, INPUT);
	
}


void loop() {
	Serial.print("Carriage hall effect: ");
	Serial.println(digitalRead(11));
	Serial.print("Spool hall effect: ");
	Serial.println(digitalRead(13));
	Serial.print("Ballast hall effect: ");
	Serial.println(digitalRead(10));
	Serial.print("Direction Sense Carriage: ");
	Serial.println(digitalRead(9));
	Serial.print("Direction Sense Spool: ");
	Serial.println(digitalRead(12));
	Serial.print("Direction Sense Ballast: ");
	Serial.println(digitalRead(2));
  Serial.println();
	
	delay(500);
}
