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

uint8_t carriageLSB =	4;
uint8_t carriageMSB =	5;
uint8_t spoolLSB =		8;
uint8_t spoolMSB =		7;
uint8_t ballastLSB =	3;
uint8_t ballastMSB =	6;

//Debounce Counters
uint8_t carriageDebounce =	0;
uint8_t spoolDebounce =		0;
uint8_t ballastDebounce =	0;

uint8_t debounceThreshhold = 3;

uint16_t delayTime = 200;

//Parallel Bus Current data:
int8_t carriageBus =	0;
int8_t spoolBus =		0;
int8_t ballastBus =		0;

//Current Hall Sensor state (hall sensors digitalRead as false when activated):
bool carriageState =	true;
bool spoolState =		true;
bool ballastState = 	true;

//debug
uint8_t counter1 = 0;

void setup() {
	Serial.begin(9600);
	
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
	updateBus(carriageHall, readPin(carriageHall, carriageSense));
	updateBus(spoolHall, readPin(spoolHall, spoolSense));
	updateBus(ballastHall, readPin(ballastHall, ballastSense));
	
	if(counter1 == 200){
		Serial.print("MSB: ");
		Serial.println(digitalRead(ballastMSB));
    Serial.print("LSB: ");
		Serial.println(digitalRead(ballastLSB));
		counter1 = 0;
	}
	counter1++;
	
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
	if(!pinRead && getDebounceCount(pin) == debounceThreshhold && getState(pin)){
		if(digitalRead(sensePin)){
			increment = 1;
		}
		else{
			increment  = -1;
		}
		updateDebounceCount(pin, -debounceThreshhold);
		updateState(pin);
	}
	//If pin read high and insufficient debounce count reached, increment debounce counter
	else if(!pinRead && getDebounceCount(pin) < debounceThreshhold && getState(pin)){
		updateDebounceCount(pin, 1);
	}
	
	else if(pinRead && getDebounceCount(pin) == debounceThreshhold && !getState(pin)){
		updateState(pin);
		updateDebounceCount(pin, -debounceThreshhold);
	}
	else if(pinRead && getDebounceCount(pin) < debounceThreshhold && !getState(pin)){
		updateDebounceCount(pin, 1);
	}
	else{
		updateDebounceCount(pin, -getDebounceCount(pin));
	}
	return increment; 
}
/*
updates the parallel bus. Given Hall Pin and increment size.
*/
void updateBus(uint8_t pin, int8_t increment){
	if(increment != 0){
		if(pin == carriageHall){
			carriageBus += increment;
			if(carriageBus > 3){
				carriageBus = 0;
			}
			else if(carriageBus < 0){
				carriageBus = 3;
			}
			writeToBus(carriageLSB, carriageMSB, carriageBus);
		}
		else if(pin == spoolHall){
			spoolBus += increment;
			if(spoolBus > 3){
				spoolBus = 0;
			}
			else if(spoolBus < 0){
				spoolBus = 3;
			}
			writeToBus(spoolLSB, spoolMSB, spoolBus);
		}
		else if(pin == ballastHall){
			ballastBus += increment;
			if(ballastBus > 3){
				ballastBus = 0;
			}
			else if(ballastBus < 0){
				ballastBus = 3;
			}
			writeToBus(ballastLSB, ballastMSB, ballastBus);
		}
	}
}
void writeToBus(uint8_t LSBPin, uint8_t MSBPin, int8_t busVal){
	switch (busVal){
		case 0:
			digitalWrite(LSBPin, LOW);
			digitalWrite(MSBPin, LOW);
			break;
		case 1:
			digitalWrite(LSBPin, HIGH);
			digitalWrite(MSBPin, LOW);
			break;
		case 2:
			digitalWrite(LSBPin, LOW);
			digitalWrite(MSBPin, HIGH);
			break;
		case 3:
			digitalWrite(LSBPin, HIGH);
			digitalWrite(MSBPin, HIGH);
			break;
	}
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

void updateDebounceCount(uint8_t pin, int8_t increment){
	if(pin == carriageHall){
		carriageDebounce += increment;
	}
	else if(pin == spoolHall){
		spoolDebounce += increment;
	}
	else if(pin == ballastHall){
		ballastDebounce += increment;
	}
}

bool getState(uint8_t pin){
	if(pin == carriageHall){
		return carriageState;
	}
	else if(pin == spoolHall){
		return spoolState;
	}
	else if(pin == ballastHall){
		return ballastState;
	}
}

void updateState(uint8_t pin){
	if(pin == carriageHall){
		carriageState = !carriageState;
	}
	else if(pin == spoolHall){
		spoolState = !spoolState;
	}
	else if(pin == ballastHall){
		ballastState = !ballastState;
	}
}
