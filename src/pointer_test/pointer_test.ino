
uint8_t lastState = 0;


void setup(){
	Serial.begin(9600);
	
	Serial.print("Val is currently:" );
  Serial.println(lastState);
	
	updateEncoder(lastState);
	
	Serial.print("Val is now: ");
  Serial.println(lastState);
}

void loop(){
	
}


void updateEncoder(uint8_t &lastState){
	
	
	uint8_t *previousState;
	
	previousState = &lastState;
	
	*previousState = 5;
	
}
