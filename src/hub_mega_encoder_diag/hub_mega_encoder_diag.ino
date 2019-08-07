/*
Submarine Arduino Mega Hub Code

Handles the transmission and reception of Serial data packets.
Writes all PWM data to the PWM Driver Board.
Reads the encoder bus.
Reads all analog sensors:
	-Water sensors
	-Temperature sensors
	-Potentiometer Feedback Sensors
Runs the control algorithm for interpreting and repositioning mechanical systems

*/

/*
Program Flow:

Declare variables, include libraries, define constants

Setup:
	-initialize Serial and Serial1
	-initialize PWM driver board
	-set pinmodes, where necessary
	
Loop:
	-Start by checking the serial bus for new packet. If present, update local setpoints with new packet data
	-begin read operations:
		-Read encoder bus, increment or decriment as necessary
		-Read Potentiometers
		-Read Temperature Sensors
		-Read water sensor
		
	-begin write operations
		-send servo writes, if new setpoints have been assigned
		-Send new headlight value, if assigned
		-Send ballast ESC write with PI control
		-Send Spool/Carriage writes:
			-Start by checking if setpoint != current spool encoder count
			-If it is different, then write appropriate direction to spool servo (don't move otherwise)
				-check current spool encoder count against carriage encoder count (with scaling factor for spool counts:carriage counts per rev of spool)
					(use iterating factor - number of times the carriage has made a full travel)
					Determine where the carriage needs to be (from spool count) vs where it is. The spool count acts as the carriage setpoint
						Assign appropriate write, if necessary 
	
	-assemble new transmit data packet
		-write new data packet to Serial bus

*/

/***********
* INCLUDES *
***********/
#include <Wire.h>
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

//Direction Sense. digitalWrite of HIGH on these directs an upward count on the encoder nano
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
const uint16_t BAUD_RATE = 					9600;
const uint8_t SPOOL_BALLAST_UPDATE_COUNT =  10;
const uint8_t SENSORS_UPDATE_COUNT =		20;
const uint8_t CONTROL_UPDATE_COUNT = 		3;
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

//Drive Motor ESC. Min is forward, max is reverse
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
int16_t spoolSetpoint = 			0;
int16_t ballastSetpoint = 			0;

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
int16_t spoolPositionCurrent = 		0;
uint16_t ballastPositionCurrent = 	0;
uint8_t motorTempCurrent = 			0;
uint8_t waterSenseCurrent = 		0;
uint8_t batteryVoltage = 			0; 

const uint8_t SUB_PACKET_SIZE = 	10;
byte currentSubData[SUB_PACKET_SIZE];

/**************************
* Spooling Lookup Indices *
***************************/
const uint16_t SPOOL_FIRST_INTERVAL = 	106;
const uint16_t SPOOL_SECOND_INTERVAL = 	212;
const uint16_t SPOOL_THIRD_INTERVAL = 	318;
const uint16_t SPOOL_FOURTH_INTERVAL = 	424;
const uint16_t SPOOL_FIFTH_INTERVAL =	530;

const uint16_t CARRIAGE_SOFT_LIMIT = 	155;
 
/**********
* GLOBALS *
**********/
bool isUpdated =					false;
uint8_t updateSpoolBallastCounter = 0;
uint8_t emagCounter = 				0;
uint8_t updateSensorsCounter = 		0;
uint8_t updateControlCounter = 		0;

int8_t spoolBusLastState = 			0;
int8_t ballastBusLastState = 		0;
int8_t carriageBusLastState = 		0;

//carriage position not sent over serial, so its here in globals
int16_t carriagePositionCurrent = 	0;

//diag
uint16_t diagCounter = 0;
String serialData = "";

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

//Setup Routine
void setup() {
	//first make sure the encoder counts a positive increase. 
    digitalWrite(CARRIAGE_SENSE, HIGH);
	digitalWrite(BALLAST_SENSE, HIGH);
	digitalWrite(SPOOL_SENSE, HIGH);
	
	//Radio/mini-sub Serial Initiation
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
		spoolSetpoint = serialData.toInt();
	}
	
	if(diagCounter == 1000){
		Serial.print("SpoolCounter: ");
		Serial.println(spoolPositionCurrent);
		Serial.print("CarriageCounter: ");
		Serial.println(carriagePositionCurrent);
		diagCounter = 0;
	}
	diagCounter++;
	/*
	Enter this in multiples of the thread refresh rate. Performs these actions:
	1. updates encoder counts for spool/ballast/carriage
	*/
	if(updateSpoolBallastCounter > SPOOL_BALLAST_UPDATE_COUNT){
		
		spoolPositionCurrent += updateEncoder(SPOOL_MSB, SPOOL_LSB, spoolBusLastState);
		ballastPositionCurrent += updateEncoder(BALLAST_MSB, BALLAST_LSB, ballastBusLastState);
		carriagePositionCurrent += updateEncoder(CARRIAGE_MSB, CARRIAGE_LSB, carriageBusLastState);
		
		updateSpoolBallastCounter = 0;
	}
	
	/*
	Enter this in multiples of the thread refresh rate. Performs these actions:
	1. Updates the ballast position with new setpoint data.
	
	*/
	if(updateControlCounter > CONTROL_UPDATE_COUNT){
		//Ballast control algorithm. setpoint increases as water is drawn in:
		if(ballastSetpoint == ballastPositionCurrent){
			pwm.setPWM(BALLAST_ESC, 0, BALLAST_CENTER);
		}
		else if(ballastSetpoint < ballastPositionCurrent){
			pwm.setPWM(BALLAST_ESC, 0, 325);
		}
		else if(ballastSetpoint > ballastPositionCurrent){
			pwm.setPWM(BALLAST_ESC, 0, 375);
		}
		
		//Now the spool. Setpoint increases as tether is unspooled:
		if(spoolSetpoint == spoolPositionCurrent){
			pwm.setPWM(SPOOL_SERVO, 0, SPOOL_CENTER);
			pwm.setPWM(CARRIAGE_SERVO, 0, CARRIAGE_CENTER);
		}
		//Spooling out
		else if(spoolSetpoint > spoolPositionCurrent){
			digitalWrite(SPOOL_SENSE, HIGH);
			pwm.setPWM(SPOOL_SERVO, 0, 357);
			setCarriage(1);
		}
		//spooling in
		else if(spoolSetpoint < spoolPositionCurrent){
			digitalWrite(SPOOL_SENSE, LOW);
			pwm.setPWM(SPOOL_SERVO, 0, 389);
			setCarriage(-1);
		}
		updateControlCounter = 0;
	}
	
	/*
	Enter this in multiples of the thread refresh rate. Peforms these actions:
	1. Gets analogRead of rudder, aft Dive, fore Dive, motor temp, water sense,
	battery voltage.
	2. Updates SubPacket 'Current' vars with data.
	*/
	if(updateSensorsCounter > SENSORS_UPDATE_COUNT){
		
		//TODO: assign offsets properly
		rudderPositionCurrent = analogRead(RUDDER_FEEDBACK) - RUDDER_FEEDBACK_CENTER;
		aftDivePositionCurrent = analogRead(AFT_DIVE_FEEDBACK) - AFT_DIVE_FEEDBACK_CENTER;
		foreDivePositionCurrent = analogRead(FORE_DIVE_FEEDBACK) - FORE_DIVE_FEEDBACK_CENTER;
		motorTempCurrent = analogRead(MOTOR_TEMP_SENSE) - MOTOR_TEMP_SENSE_CENTER;
		waterSenseCurrent = analogRead(WATER_SENSE) - WATER_SENSE_CENTER;
		batteryVoltage = analogRead(BATTERY_VOLTAGE_SENSE) - BATTERY_VOLTAGE_SENSE_CENTER;
		
		updateSensorsCounter = 0;
	}
	
	//update the counters:
	updateSpoolBallastCounter++;
	updateSensorsCounter++;
	updateControlCounter++;
	
	//and the delay:
	delayMicroseconds(THREAD_FREQ);
	
}
/*
updateEncoder() - takes the MSB and LSB pins and returns the amount by which 
the encoded system has moved - an increment or decriment.
*/
int8_t updateEncoder(uint8_t MSB, uint8_t LSB, int8_t &lastState){
	int8_t currentState = 0;
	int8_t *previousState;
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
		if(currentState == 3 && *previousState == 0){
			increment = -1;
			*previousState = 3;
		}
		else if(currentState == 0 && *previousState == 3){
			increment = 1;
			*previousState = 0;
		}
		else{
			increment = currentState - *previousState;
			*previousState = currentState;
		}
		
	}
	return increment;
}

void setCarriage(int8_t spoolingState){
	//spoolingState is -1 when spooling in, 1 when spooling out
	if(spoolPositionCurrent < SPOOL_FIRST_INTERVAL){
		carriagePWMSet(spoolingState);
	}
	else if(spoolPositionCurrent >= SPOOL_FIRST_INTERVAL && spoolPositionCurrent < SPOOL_SECOND_INTERVAL){
		carriagePWMSet(-1 * spoolingState);
	}
	else if(spoolPositionCurrent >= SPOOL_SECOND_INTERVAL && spoolPositionCurrent < SPOOL_THIRD_INTERVAL){
		carriagePWMSet(spoolingState);
	}
	else if(spoolPositionCurrent >= SPOOL_THIRD_INTERVAL && spoolPositionCurrent < SPOOL_FOURTH_INTERVAL){
		carriagePWMSet(-1 * spoolingState);
	}
	else if(spoolPositionCurrent >= SPOOL_FOURTH_INTERVAL && spoolPositionCurrent < SPOOL_FIFTH_INTERVAL){
		carriagePWMSet(spoolingState);
	}
	else if(spoolPositionCurrent >= SPOOL_FIFTH_INTERVAL){
		carriagePWMSet(-1 * spoolingState);
	}
}
void carriagePWMSet(int8_t spoolingState){
	
	if(spoolingState == -1 && carriagePositionCurrent >= 0){
		pwm.setPWM(CARRIAGE_SERVO, 0, CARRIAGE_MIN);
		digitalWrite(CARRIAGE_SENSE, LOW);
	}
	else if(spoolingState == 1 && carriagePositionCurrent <= CARRIAGE_SOFT_LIMIT){
		pwm.setPWM(CARRIAGE_SERVO, 0, CARRIAGE_MAX);
		digitalWrite(CARRIAGE_SENSE, HIGH);
	}	
}
