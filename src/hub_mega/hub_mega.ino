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

//Setup Routine
void setup() {
    Serial.begin(BAUD_RATE);     //USB
    Serial1.begin(BAUD_RATE);    //Radio/mini-sub
	
	//Initialize the PWM Driver Board
	pwm.begin();
	pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
	
	delay(200);
	
	//turn on status LED to indicate ready operation
	pwm.setPWM(STATUS_LED, 0, STATUS_MAX);

}

//Loop Routine
void loop() {
	//enter if the sub has received a serial packet from the mini sub
    if(Serial1.available()>0){
		//wait a few ms for the data to be received
		delay(3);
		
		//Read the serial data
		for(uint8_t i = 0; i < STATION_PACKET_SIZE; i++){
			currentStationData[i] = Serial.read();
		}
		
		/*
		Assign to relevant variables with unit conversion
		CurrentStationData follows the units laid out in the spreadsheet.
		local setpoints are all in pwm duty cycle, except spool and ballast setpoints
		which are given as direct encoder counts
		*/
		driveDelta = currentStationData[0];
		driveSetpoint = DRIVE_CENTER + driveDelta;
		
		rudderDelta = currentStationData[1];
		rudderSetpoint = RUDDER_CENTER + rudderDelta;
		
		aftDiveDelta = currentStationData[2];
		aftDiveSetpoint = AFT_DIVE_CENTER + aftDiveDelta;
		
		foreDiveDelta = currentStationData[3];
		foreDiveSetpoint = FORE_DIVE_CENTER + foreDiveDelta;
		
		headLightSetpoint = currentStationData[4] * 40;
		
		spoolSetpoint = 0;
		spoolSetpoint = currentStationData[5];
		spoolSetpoint = spoolSetpoint << 8;
		spoolSetpoint = spoolSetpoint | currentStationData[6];
		
		ballastSetpoint = 0;
		ballastSetpoint = currentStationData[7];
		ballastSetpoint = ballastSetpoint << 8;
		ballastSetpoint = ballastSetpoint | currentStationData[8];
		
		/*
		Now construct the return serial packet and write it
		this way the sub only transmits data when it receives it,
		which allows for external timing control
		*/
		
		currentSubData[0] = rudderPositionCurrent;
		currentSubData[1] = aftDivePositionCurrent;
		currentSubData[2] = foreDivePositionCurrent;
		currentSubData[3] = spoolPositionCurrent >> 8;
		currentSubData[4] = spoolPositionCurrent;
		currentSubData[5] = ballastPositionCurrent >> 8;
		currentSubData[6] = ballastPositionCurrent;
		currentSubData[7] = motorTempCurrent;
		currentSubData[8] = waterSenseCurrent;
		
		//Write the serial data:
		Serial1.write(currentSubData, SUB_PACKET_SIZE);
		
		isUpdated = true;
	}
	
	//Update the setpoints if new data has been received:
	if(isUpdated){
		/*
		Direct PWM Writes: drive, rudder, aft Dive, fore dive, headlights
		Ballast and Spool handled in separate algo
		*/
		if(!(driveSetpoint > DRIVE_MAX) && !(driveSetpoint < DRIVE_MIN)){
			pwm.setPWM(DRIVE_ESC, 0, driveSetpoint);	
		}
		if(!(rudderSetpoint > RUDDER_MAX) && !(rudderSetpoint < RUDDER_MIN)){
			pwm.setPWM(RUDDER_SERVO, 0, rudderSetpoint);	
		}
		if(!(aftDiveSetpoint > AFT_DIVE_MAX) && !(aftDiveSetpoint < AFT_DIVE_MIN)){
			pwm.setPWM(AFT_DIVE_SERVO, 0, aftDiveSetpoint);	
		}
		if(!(foreDiveSetpoint > FORE_DIVE_MAX) && !(foreDiveSetpoint < FORE_DIVE_MIN)){
			pwm.setPWM(FORE_DIVE_SERVO, 0, foreDiveSetpoint);	
		}
		if(!(headLightSetpoint > HEADLIGHT_MAX) && !(headLightSetpoint < HEADLIGHT_MIN)){
			pwm.setPWM(HEADLIGHTS, 0, headLightSetpoint);	
		}
		
		isUpdated = false;
	}
	
	/*
	Enter this in multiples of the thread refresh rate. Performs these actions:
	1. updates encoder counts for spool/ballast/carriage
	2. checks setpoints against current values
	3. assigns appropriate pwm writes as necessary
	4. Assigns appropriate direction sense as necessary
	
	Also toggles the electromagnet if a spool change from 0 occurs
	*/
	if(updateSpoolBallastCounter > SPOOL_BALLAST_UPDATE_COUNT){
		
		
		
		updateSpoolBallastCounter = 0;
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
	
	//and the delay:
	delayMicroseconds(THREAD_FREQ);
	
}
