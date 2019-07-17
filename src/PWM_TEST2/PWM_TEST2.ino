
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

uint8_t servonum = 0;
uint16_t armPulse = 331;
uint16_t setPulse = 0;
uint8_t mode = 0;

String serialData = "";
 
void setup() {
	Serial.begin(9600);
	Serial.println("PWM I2C Board Tester");

	pinMode(52, OUTPUT);
	pwm.begin();
  
	pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates

	delay(10);
  
	Serial.println("Program Ready");
}

void loop() {
	if(Serial.available() > 0){
		/*
		MODES:
		0 = Init
		1 = Set PWM Port
		2 = Set PWM Pulsewidth to PWM Port from Mode 1
		
		*/
		serialData = Serial.readString();
		if(mode == 0){
			//init mode
			if(serialData.equals("Set Port")){
				mode = 1;
				Serial.println("In Port Setting Mode: Type a valid port number now");
			}
			else if(serialData.equals("Set Pulse")){
				mode = 2;
				Serial.println("In Pulse Setting mode: Type a valid Pulse number now");
			}
		}
		else if(mode == 1){
			if(serialData.equals("Set New")){
				mode = 0;
			}
			else{
				//here, Select the port using conversion to uint_8t
				servonum = serialData.toInt();
				Serial.print("PWM Port set to: ");
				Serial.println(servonum);	
			}
			
		}
		else if(mode == 2){
			if(serialData.equals("Set New")){
				mode = 0;
			}
			else{
				//here, perform conversion to uint_16t of any input data as this is direct pulse width to servo writes
				setPulse = serialData.toInt();
				Serial.print("Writing Pulsewidth of: ");
				Serial.print(setPulse);
				Serial.print(" to Port Pin: ");
				Serial.println(servonum);
			
				//PWM write:
				pwm.setPWM(servonum, 0, setPulse);
			}
		}
		else if(serialData.equals("print")){
			//turn on temp sensor for reads
			digitalWrite(52, HIGH);
			
			Serial.print("Water Sensor: ");
			Serial.println(analogRead(5));
			Serial.print("Motor Temp: ");
			Serial.println(analogRead(0));
			Serial.print("Rudder Potentiometer: ");
			Serial.println(analogRead(4));
			Serial.print("Aft Dive Potentiometer: ");
			Serial.println(analogRead(1));
			Serial.print("Fore Dive Potentiometer: ");
			Serial.println(analogRead(12));
			Serial.print("Battery Voltage: ");
			Serial.println(analogRead(15));
			
			//turn off temp sensor after reading
			digitalWrite(52, LOW); 
			
		}
	}	
	delay(20);
}


