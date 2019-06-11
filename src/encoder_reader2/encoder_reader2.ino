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
uint8_t carriageDebounce = 0;
uint8_t spoolDebounce = 0;
uint8_t ballastDebounce = 0;

uint8_t debounceThreshhold = 3;

uint8_t delayTime = 100;


//
void setup() {
	//Configure the GPIO as INPUT/OUTPUT
	pinMode(carriageHall, INPUT);
	pinMode(spoolHall, INPUT);
	pinMode(ballastHall, INPUT);
	
	pinMode(carriageSense, INPUT);
	pinMode(spoolSense, INPUT);
	pinMode(ballastSense, INPUT);
	
	pinMode(carriageLSB, OUTPUT);
	pinMode(carriageMSB, OUTPUT);
	pinMode(spoolLSB, OUTPUT);
	pinMode(spoolMSB, OUTPUT);
	pinMode(ballastLSB, OUTPUT);
	pinMode(ballastMSB, OUTPUT);
	
}

/*
As the code loops, the mega needs to check every hall effect pin for a rising change of state (with debounce). 
If there is a change, check the corresponding direction sense pin. Then increment the corresponding MSB/LSB
*/
void loop() {
	
	
	
	delayMicroseconds(delayTime);
}
/*
reads one of the hall effect pins. Performs a debounce check. 
Returns the increment amount int8_t: -1, 0, or 1
*/
int8_t readPin(uint8_t pin, uint8_t sensePin){
	int8_t increment = 0;
	bool pinRead = digitalRead(pin);
	//If pin read high and correct debounce count reached, increment parallel bus
	if(pinRead && getDebounceCount(pin) == debounceThreshhold){
		digitalRead(sensePin);
	}
	//If pin read high and insufficient debounce count reached, increment debounce counter
	else if(pinRead && getDebounceCount(pin) < debounceThreshhold)
	
	
	return increment; 
}
/*
updates the parallel bus. Given LSB, MSB, and increment size.
*/
void updateBus(uint8_t LSB, uint8_t MSB, int8_t increment){
	
}

uint8_t getDebounceCount(uint8_t pin){
	if(pin == carriageHall){
		return carriageDebounce;
	}
	else if(pin == spoolHall){
		return spoolDebounce;
	}
	else if(pin == ballastHall){
		return ballastDebounce;
	}
}

void updateDebounceCount(uint8_t pin){
	if(pin == carriageHall){
		carriageDebounce++;
	}
	else if(pin == spoolHall){
		spoolDebounce++;
	}
	else if(pin == ballastHall){
		ballastDebounce++;
	}
}



