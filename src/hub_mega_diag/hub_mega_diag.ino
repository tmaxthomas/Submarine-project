/*
	Mega Diagnostic Routine for Checking Electromechanical Functionality
*/

/***********
* INCLUDES *
***********/
#include <Wire.h>
#include "../common.h"
#include <Adafruit_PWMServoDriver.h>

/******************
 * MACROS/DEFINES *
 *****************/
 
 #define BAUD_RATE 9600

/*************
* PWM LIMITS *
*************/

//Carriage. PWM below Center sends carriage towards aft. 
#define CARRIAGE_MIN		330
#define CARRIAGE_CENTER 	363
#define CARRIAGE_MAX		396

//Spool. PWM below Center Spools out the tether
#define SPOOL_MIN 			340
#define SPOOL_CENTER 		374
#define SPOOL_MAX 			410

//Rudder Servo. PWM below Center Steers sub to right
#define RUDDER_MIN 			340 
#define RUDDER_CENTER 		395
#define RUDDER_MAX 			470

//Aft Dive Servo. PWM below Center points planes downwards as if to Dive.
#define AFT_DIVE_MIN 		260
#define AFT_DIVE_CENTER 	320
#define AFT_DIVE_MAX 		380

//Fore Dive Servo. PWM below Center points planes downwards as if to surface.
#define FORE_DIVE_MIN 		260
#define FORE_DIVE_CENTER 	355
#define FORE_DIVE_MAX 		425

//Headlights. 0 is off, 4096 max on
#define HEADLIGHT_MIN 		0
#define HEADLIGHT_MAX 		4096

//Drive Motor ESC. TODO: check directions
#define DRIVE_MIN 			300
#define DRIVE_CENTER 		350
#define DRIVE_MAX 			400

//Ballast Motor ESC. PWM below center pushes water out of ballast. 
#define BALLAST_MIN 		300
#define BALLAST_CENTER 		350
#define BALLAST_MAX 		400



/***********
 * STRUCTS *
 ***********/

struct encoder_t {
    uint16_t pos, set, old;
    bool dir;
} spool, ballast, shuttle;

/***********
 * GLOBALS *
 ***********/

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();


/*************
 * FUNCTIONS *
 *************/



/**************
 * SETUP/LOOP *
 **************/

void setup() {
    Serial.begin(BAUD_RATE);     //USB
	
	//Initialize the PWM Driver Board
	pwm.begin();
	pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
	
}

void loop() {
   if(Serial.available() > 0){
	   
}
