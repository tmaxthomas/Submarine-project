
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

//Define Pins of PWM Board

const uint8_t AFT_DIVE_SERVO =	0;
const uint8_t RUDDER_SERVO = 	1;
const uint8_t CARRIAGE_SERVO = 	2;
const uint8_t SPOOL_SERVO = 	3;
const uint8_t HEADLIGHTS = 		4;
const uint8_t STATUS_LED =		5;
const uint8_t DRIVE_ESC = 		7;
const uint8_t FORE_DIVE_SERVO =	14;
const uint8_t BALLAST_ESC =		15;
const uint8_t EMAG = 			47;


uint16_t pulse = 0;
bool isLoop = true;
String serialData = "";
uint8_t state = 0;
 
void setup() {
	Serial.begin(9600);
	Serial.println("SUBMARINE DIAG PROGRAM STARTING");

	pinMode(EMAG, OUTPUT);
	
	pwm.begin();
	pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates

	//delay(2500);
  
	Serial.println("Arming Ballast ESC...");
	//delay(1000);
	pwm.setPWM(BALLAST_ESC, 0, 350);
//	delay(2000);
	Serial.println("Ballast Armed");
	delay(300);
	Serial.println("Arming Drive ESC...");
	//delay(1000);
	pwm.setPWM(DRIVE_ESC, 0, 350);
//	delay(2000);
	Serial.println("Drive Armed");
//	delay(800);
  
	Serial.println("Zeroing Planes");
	//delay(500);
	pwm.setPWM(AFT_DIVE_SERVO, 0, 320);
	delay(200);
	pwm.setPWM(FORE_DIVE_SERVO, 0, 390);
	delay(200);
	pwm.setPWM(RUDDER_SERVO, 0, 400);
  
	//ensure spool is not moving
	pwm.setPWM(SPOOL_SERVO, 0, 373);
	pwm.setPWM(CARRIAGE_SERVO, 0, 367);
	
	delay(1000);
	Serial.println("READY");
	pwm.setPWM(STATUS_LED, 0 , 4000);
  
}

void loop() {
	if(Serial.available() > 0){
		serialData = Serial.readString();
		
		if(serialData.equals("Drive")){
			Serial.println("In Drive Mode");
			while(isLoop){
				pulse = getPulse();
				if(pulse == 0){
					isLoop = false;
				}
				else{
					pwm.setPWM(DRIVE_ESC, 0, pulse);
				}
			}
			isLoop = true;
			pulse = 350;
			Serial.println("Exiting drive mode");
		}
		
		else if(serialData.equals("Ballast")){
			Serial.println("In Ballast Mode");
			while(isLoop){
				pulse = getPulse();
				if(pulse == 0){
					isLoop = false;
				}
				else{
					pwm.setPWM(BALLAST_ESC, 0, pulse);
				}
			}
			isLoop = true;
			pulse = 350;
			Serial.println("Exiting Ballast mode");
		}
		
		else if(serialData.equals("Aft Dive")){
			Serial.println("In Aft Dive Mode");
			while(isLoop){
				pulse = getPulse();
				if(pulse == 0){
					isLoop = false;
				}
				else{
					pwm.setPWM(AFT_DIVE_SERVO, 0, pulse);
				}
			}
			isLoop = true;
			pulse = 350;
			Serial.println("Exiting Aft Dive mode");
		}
		
		else if(serialData.equals("Fore Dive")){
			Serial.println("In Fore Dive Mode");
			while(isLoop){
				pulse = getPulse();
				if(pulse == 0){
					isLoop = false;
				}
				else{
					pwm.setPWM(FORE_DIVE_SERVO, 0, pulse);
				}
			}
			isLoop = true;
			pulse = 350;
			Serial.println("Exiting Fore Dive mode");
		}
		
		else if(serialData.equals("Rudder")){
			Serial.println("In Rudder Mode");
			while(isLoop){
				pulse = getPulse();
				if(pulse == 0){
					isLoop = false;
				}
				else{
					pwm.setPWM(RUDDER_SERVO, 0, pulse);
				}
			}
			isLoop = true;
			pulse = 350;
			Serial.println("Exiting Rudder mode");
		}
		
		else if(serialData.equals("Headlights")){
			Serial.println("In Headlights Mode");
			while(isLoop){
				pulse = getPulse();
				if(pulse == 0){
					isLoop = false;
				}
				else{
					pwm.setPWM(HEADLIGHTS, 0, pulse);
				}
			}
			isLoop = true;
			pulse = 350;
			Serial.println("Exiting headlights mode");
		}
		
		else if(serialData.equals("EMAG")){
			digitalWrite(EMAG, HIGH);
			delay(1000);
			digitalWrite(EMAG, LOW);
		}
		
		else if(serialData.equals("Spool")){
			Serial.println("Entering Spool Mode");
			while(isLoop){
				if(Serial.available() > 0){
					serialData = Serial.readString();
					//spool in
					if(serialData.equals("w")){
						pwm.setPWM(SPOOL_SERVO, 0, 390);
						if(state == 1){
							pwm.setPWM(CARRIAGE_SERVO, 0, 330);
							state = 0;
						}
						else{
							pwm.setPWM(CARRIAGE_SERVO, 0, 396);
							state = 1;
						}
					}
					//stop spooling
					else if(serialData.equals("s")){
						pwm.setPWM(SPOOL_SERVO, 0, 374);
						pwm.setPWM(CARRIAGE_SERVO, 0, 363);
						state = 0;
					}
					//spool out
					else if(serialData.equals("r")){
						pwm.setPWM(SPOOL_SERVO, 0, 355);
						if(state == 1){
							pwm.setPWM(CARRIAGE_SERVO, 0, 330);
							state = 0;
						}
						else{
							pwm.setPWM(CARRIAGE_SERVO, 0, 396);
							state = 1;
						}
					}
					else if(serialData.equals("f")){
						if(state == 1){
							pwm.setPWM(CARRIAGE_SERVO, 0, 330);
							state = 0;
						}
						else{
							pwm.setPWM(CARRIAGE_SERVO, 0, 396);
							state = 1;
						}
					}
					else if(serialData.equals("Stop")){
						isLoop = false;
						Serial.println("Exiting Spool Mode");
						pwm.setPWM(SPOOL_SERVO, 0, 374);
						pwm.setPWM(CARRIAGE_SERVO, 0, 364);
					}
				}
			}
			isLoop = true;
		}
	}
	delay(5);
}

uint16_t getPulse(){
	uint16_t data = 0;
	while(Serial.available() == 0);
	delay(10);
	serialData = Serial.readString();
	data = serialData.toInt();
	return data;
}
