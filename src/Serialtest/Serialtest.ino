const uint8_t STATION_PACKET_SIZE = 10;
byte currentStationData[STATION_PACKET_SIZE];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
	if(Serial.available() == STATION_PACKET_SIZE){
		for(uint8_t i = 0; i < STATION_PACKET_SIZE; i++){
			currentStationData[i] = Serial.read();
		}
		Serial.write(currentStationData, STATION_PACKET_SIZE);
	}
	delay(1);
}
