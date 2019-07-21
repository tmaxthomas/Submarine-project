
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();


uint8_t state = 0;
uint16_t pulse = 330;

String serialData = "";
 
void setup() {
  Serial.begin(9600);
  Serial.println("PWM I2C Board Tester");

  pwm.begin();
  
  pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates

  delay(500);
  
  pwm.setPWM(3, 0, 374);
  pwm.setPWM(2, 0, 363);
  
  Serial.println("READY");
  
}

void loop() {
	if(Serial.available() > 0){
		serialData = Serial.readString();
		
		if(serialData.equals("w")){
			pwm.setPWM(3, 0, 390);
			pwm.setPWM(2, 0, pulse);
		}
		else if(serialData.equals("s")){
			pwm.setPWM(3, 0, 374);
			pwm.setPWM(2, 0, 363);
		}
		else if(serialData.equals("r")){
			pwm.setPWM(3, 0, 355);
			if(state == 0){
				pulse = 396;
				pwm.setPWM(2, 0, pulse);
				state = 1;
			}
			else{
				pulse = 330;
				pwm.setPWM(2, 0, pulse);
				state = 0;
			}
		}
		else if(serialData.equals("f")){
			if(state == 0){
				pulse = 396;
				pwm.setPWM(2, 0, pulse);
				state = 1;
			}
			else{
				pulse = 330;
				pwm.setPWM(2, 0, pulse);
				state = 0;
			}
		}
	}
	delay(5);
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
