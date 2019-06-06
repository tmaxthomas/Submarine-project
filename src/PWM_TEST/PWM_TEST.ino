
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // this is the 'maximum' pulse length count (out of 4096)

uint8_t servonum = 0;
uint16_t armPulse = 331;
uint16_t setPulse = 0;
uint8_t mode = 0;

String serialData = "";
 
void setup() {
  Serial.begin(9600);
  Serial.println("PWM I2C Board Tester");

  pwm.begin();
  
  pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates

  delay(10);
  
  Serial.println("Arming ESC(s)...");
  //ARM ESC HERE:
 // pwm.setPWM(servonum, 0, armPulse);
  delay(4000);
  Serial.println("ESC(s) Armed, Program ready!");
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
	}	
	delay(20);
}





// you can use this function if you'd like to set the pulse length in seconds
// e.g. setServoPulse(0, 0.001) is a ~1 millisecond pulse width. its not precise!
void setServoPulse(uint8_t n, double pulse) {
  double pulselength;
  
  pulselength = 1000000;   // 1,000,000 us per second
  pulselength /= 60;   // 60 Hz
  Serial.print(pulselength); Serial.println(" us per period"); 
  pulselength /= 4096;  // 12 bits of resolution
  Serial.print(pulselength); Serial.println(" us per bit"); 
  pulse *= 1000000;  // convert to us
  pulse /= pulselength;
  Serial.println(pulse);
  pwm.setPWM(n, 0, pulse);
}
