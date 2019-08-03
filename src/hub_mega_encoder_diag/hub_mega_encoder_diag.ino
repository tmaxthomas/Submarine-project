/*
Submarine Arduino Mega Encoder Diag Code

*/

/***********
* INCLUDES *
***********/
#include <Wire.h>
#include "../common.h"
#include <Adafruit_PWMServoDriver.h>

/******************
* PIN DEFINITIONS *
******************/
//PWM Board
const uint8_t AFT_DIVE_SERVO =	0;
const uint8_t RUDDER_SERVO = 	1;
const uint8_t CARRIAGE_SERVO = 	2;
const uint8_t SPOOL_SERVO = 	3;
const uint8_t HEADLIGHTS = 		4;
const uint8_t STATUS_LED =		5;
const uint8_t DRIVE_ESC = 		7;
const uint8_t FORE_DIVE_SERVO =	14;
const uint8_t BALLAST_ESC =		15;

//Parallel Bus:
const uint8_t CARRIAGE_MSB = 	53;
const uint8_t CARRIAGE_LSB = 	50;
const uint8_t SPOOL_MSB = 		51;
const uint8_t SPOOL_LSB = 		48;
const uint8_t BALLAST_MSB = 	49;
const uint8_t BALLAST_LSB = 	46;
//Direction Sense
const uint8_t CARRIAGE_SENSE =	44;
const uint8_t SPOOL_SENSE = 	45;
const uint8_t BALLAST_SENSE = 	43;

//Analog Input
const uint8_t WATER_SENSE = 			5;
const uint8_t MOTOR_TEMP_SENSE = 		0;
const uint8_t RUDDER_FEEDBACK = 		4;
const uint8_t AFT_DIVE_FEEDBACK = 		1;
const uint8_t FORE_DIVE_FEEDBACK = 		12;
const uint8_t BATTERY_VOLTAGE_SENSE = 	15;

//Other
const uint8_t EMAG = 			47;

/******************
 * MACROS/DEFINES *
 *****************/
const uint16_t BAUD_RATE = 					115200;
const uint8_t SPOOL_BALLAST_UPDATE_COUNT =  20;
const uint8_t SENSORS_UPDATE_COUNT =		20;
const uint16_t THREAD_FREQ = 				500;

/*************
* PWM LIMITS *
*************/
//Carriage. PWM below Center sends carriage towards aft. 
const uint16_t CARRIAGE_MIN = 		330;
const uint16_t CARRIAGE_CENTER =  	364;
const uint16_t CARRIAGE_MAX	=		396;

//Spool. PWM below Center Spools out the tether
const uint16_t SPOOL_MIN =			340;
const uint16_t SPOOL_CENTER = 		374;
const uint16_t SPOOL_MAX =			410;

//Rudder Servo. PWM below Center Steers sub to right
const uint16_t RUDDER_MIN =			340;
const uint16_t RUDDER_CENTER =		400;
const uint16_t RUDDER_MAX =			470;

//Aft Dive Servo. PWM below Center points planes downwards as if to Dive.
const uint16_t AFT_DIVE_MIN =		260;
const uint16_t AFT_DIVE_CENTER =	320;
const uint16_t AFT_DIVE_MAX =		380;

//Fore Dive Servo. PWM below Center points planes downwards as if to surface.
const uint16_t FORE_DIVE_MIN =		290;
const uint16_t FORE_DIVE_CENTER =	390;
const uint16_t FORE_DIVE_MAX =		460;

//Headlights. 0 is off, 4095 max on
const uint16_t HEADLIGHT_MIN =		0;
const uint16_t HEADLIGHT_MAX =		4095;

//Drive Motor ESC. TODO: check directions
const uint16_t DRIVE_MIN =			280;
const uint16_t DRIVE_CENTER =		350;
const uint16_t DRIVE_MAX =			420;

//Ballast Motor ESC. PWM below center pushes water out of ballast. 
const uint16_t BALLAST_MIN =		300;
const uint16_t BALLAST_CENTER =		350;
const uint16_t BALLAST_MAX =		400;

//Status LED. 0 is off, 4095 is max on
const uint16_t STATUS_MIN = 		0;
const uint16_t STATUS_MAX = 		4095;

/******************
* FEEDBACK LIMITS *
******************/
const uint16_t WATER_SENSE_CENTER =				0;
const uint16_t MOTOR_TEMP_SENSE_CENTER = 		0;
const uint16_t RUDDER_FEEDBACK_CENTER = 		0;
const uint16_t AFT_DIVE_FEEDBACK_CENTER = 		0;
const uint16_t FORE_DIVE_FEEDBACK_CENTER = 		0;
const uint16_t BATTERY_VOLTAGE_SENSE_CENTER = 	0;

/***************************
* SEND/RECEIVE PACKET DATA *
***************************/
//StationPacket: Setpoint Data from the groundstation (start at Neutral on all)
uint16_t driveSetpoint = 			350;
uint16_t rudderSetpoint = 			400;
uint16_t aftDiveSetpoint = 			320;
uint16_t foreDiveSetpoint = 		390;
uint16_t headLightSetpoint = 		0;
uint16_t spoolSetpoint = 			0;
uint16_t ballastSetpoint = 			0;

//temp values used for unit conversion in setpoint assignment
int8_t driveDelta = 				0;
int8_t rudderDelta = 				0;
int8_t aftDiveDelta = 				0;
int8_t foreDiveDelta = 				0;
	
const uint8_t STATION_PACKET_SIZE = 9;
byte currentStationData[STATION_PACKET_SIZE];

//SubPacket: Current Running Data of the Submarine. units match those of SubPacket in spreadsheet
int8_t rudderPositionCurrent = 		0;
int8_t aftDivePositionCurrent = 	0;
int8_t foreDivePositionCurrent = 	0;
uint16_t spoolPositionCurrent = 	0;
uint16_t ballastPositionCurrent = 	0;
uint8_t motorTempCurrent = 			0;
uint8_t waterSenseCurrent = 		0;
uint8_t batteryVoltage = 			0; 

const uint8_t SUB_PACKET_SIZE = 	10;
byte currentSubData[SUB_PACKET_SIZE];

/**********
* GLOBALS *
**********/
bool isUpdated =					false;
uint8_t updateSpoolBallastCounter = 0;
uint8_t emagCounter = 				0;
uint8_t updateSensorsCounter = 		0;

uint8_t spoolBusLastState = 		0;
uint8_t ballastBusLastState = 		0;
uint8_t carriageBusLastState = 		0;

//carriage position not sent over serial, so its here in globals
uint16_t carriagePositionCurrent = 	0;


//diagnostics variables
String serialData = "";
bool isLoop = true;
uint16_t pulse = 0;
uint8_t counter = 0;


//Setup Routine
void setup() {
    //Enable Diag Serial
	Serial.begin(BAUD_RATE);    
	
	//Initialize the PWM Driver Board
	pwm.begin();
	pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
	
	delay(200);
	
	//pinmode setup for DIO pins
	pinMode(CARRIAGE_SENSE, OUTPUT);
	pinMode(SPOOL_SENSE, OUTPUT);
	pinMode(BALLAST_SENSE, OUTPUT);
	
	pinMode(EMAG, OUTPUT);
	
	pinMode(CARRIAGE_MSB, INPUT);
	pinMode(CARRIAGE_LSB, INPUT);
	pinMode(SPOOL_MSB, INPUT);
	pinMode(SPOOL_LSB, INPUT);
	pinMode(BALLAST_MSB, INPUT);
	pinMode(BALLAST_LSB, INPUT);
	
	//turn on status LED to indicate ready operation
	pwm.setPWM(STATUS_LED, 0, STATUS_MAX);
}

//Loop Routine
void loop() {
	
	if(Serial.available() > 0){
		serialData = Serial.readString();
		
		if(serialData.equals("w")){
			digitalWrite(BALLAST_SENSE, HIGH);
			pwm.setPWM(BALLAST_ESC, 0, 330);
		}	
		else if(serialData.equals("s")){
			digitalWrite(BALLAST_SENSE, LOW);
			pwm.setPWM(BALLAST_ESC, 0, 370);
		}
		else if(serialData.equals("stop")){
			pwm.setPWM(BALLAST_ESC, 0, BALLAST_CENTER);
		}
	}
	
	ballastPositionCurrent += updateEncoder(BALLAST_MSB, BALLAST_LSB, ballastBusLastState);
	delay(10);
	if(counter == 20){
		Serial.print("Current Ballast Counter: ");
		Serial.println(ballastPositionCurrent);
		counter = 0;
	}
	counter++;
	
}
/*
updateEncoder() - takes the MSB and LSB pins and returns the amount by which 
the encoded system has moved - an increment or decriment.
*/
int8_t updateEncoder(uint8_t MSB, uint8_t LSB, uint8_t &lastState){
	uint8_t currentState = 0;
	uint8_t *previousState;
	previousState = &lastState;
	int8_t increment = 0;
	if(digitalRead(MSB) && digitalRead(LSB)){
		currentState = 3;
	}
	else if(digitalRead(MSB) && !digitalRead(LSB)){
		currentState = 2;
	}
	else if(!digitalRead(MSB) && digitalRead(LSB)){
		currentState = 1;
	}
	else if(!digitalRead(MSB) && !digitalRead(LSB)){
		currentState = 0;
	}
	
	if(currentState != *previousState){
		increment = currentState - *previousState;
		*previousState = currentState;
		return increment;
	}
	return 0;
}

uint16_t getPulse(){
	uint16_t data = 0;
	while(Serial.available() == 0);
	delay(10);
	serialData = Serial.readString();
	data = serialData.toInt();
	return data;
}
