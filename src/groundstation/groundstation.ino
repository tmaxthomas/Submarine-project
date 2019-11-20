/*
Groundstation Operation Code.
Arduino receives a packet over serial from the computer.
Packet is then converted to an SPI radio packet and transmitted as such.
Arduino then waits for ack packet with the submarine data.
Received ack packet is converted to Serial and sent to the computer.


Wiring:

Radio    Arduino
CE    -> 9
CSN   -> 10 (Hardware SPI SS)
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
IRQ   -> No connection
VCC   -> No more than 3.6 volts
GND   -> GND


*/

#include <SPI.h>
#include <NRFLite.h>

const static uint8_t RADIO_ID = 1;
const static uint8_t DESTINATION_RADIO_ID = 0;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

const uint8_t LED_PIN = 2;

//StationPacket - packet received from Base Station
struct StationPacket{
	int8_t driveSetpoint;
	int8_t rudderSetpoint;
	int8_t aftDiveSetpoint;
	int8_t foreDiveSetpoint;
	uint8_t headLightSetpoint;
	uint16_t spoolSetpoint;
	uint16_t ballastSetpoint;
	uint8_t stationPacketCheck;
};

//SubPacket - packet to be sent from sub
struct SubPacket{
	int8_t rudderPosition;
	int8_t aftDivePosition;
	int8_t foreDivePosition;
	uint16_t spoolPosition;
	uint16_t ballastPosition;
	uint8_t motorTemp;
	uint8_t waterSense;
	uint8_t batteryVoltage;
	uint8_t subPacketCheck;
	
};

NRFLite _radio;

/*Current Sub Running Data:
Variables holding the latest received operational data from the sub.
Assigned to and transmitted by ack packets.
*/

const uint8_t SUB_PACKET_SIZE = 	11;
byte currentSubData[SUB_PACKET_SIZE];


/*Current Station Setpoint Data
Variables holding the latest received setpoint data from the base station.
Written over the serial bus once received
*/

const uint8_t STATION_PACKET_SIZE = 10;
byte currentStationData[STATION_PACKET_SIZE];

bool isValidPacket = false;
int availablePackets = 0;

void setup(){
	//init serial
    Serial.begin(9600);
	//init radio
    _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN);
	
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW);
}

void loop(){
	
	if(!isValidPacket){
		while(Serial.available() != 0){
			Serial.read();
			delayMicroseconds(300);
		}
		isValidPacket = true;
	}
	
	//Enter if data received from computer over serial
	if(Serial.available() == STATION_PACKET_SIZE){
		
		isValidPacket = false;
		
		for(uint8_t i = 0; i < STATION_PACKET_SIZE; i++){
			currentStationData[i] = Serial.read();
			if(((currentStationData[i] == ((uint8_t)10)) || (currentStationData[i] == ((uint8_t)30))) && i == (STATION_PACKET_SIZE-1)){
				isValidPacket = true;
			}
		}
		if(isValidPacket){
			StationPacket stationData;
			stationData.driveSetpoint = currentStationData[0];
			stationData.rudderSetpoint = currentStationData[1];
			stationData.aftDiveSetpoint = currentStationData[2];
			stationData.foreDiveSetpoint = currentStationData[3];
			stationData.headLightSetpoint = currentStationData[4];
			stationData.spoolSetpoint = ((uint16_t)currentStationData[5]) << 8;
			stationData.spoolSetpoint = stationData.spoolSetpoint | ((uint16_t)currentStationData[6]);
			stationData.ballastSetpoint = ((uint16_t)currentStationData[7]) << 8;
			stationData.ballastSetpoint = stationData.ballastSetpoint | ((uint16_t)currentStationData[8]);
			stationData.stationPacketCheck = currentStationData[9];
		
			_radio.send(DESTINATION_RADIO_ID, &stationData, sizeof(stationData));
			digitalWrite(LED_PIN, HIGH);
		}
		else{
			while(Serial.available() != 0){
				Serial.read();
			}
			digitalWrite(LED_PIN, LOW);
		}
		
	}
	else if(Serial.available() > STATION_PACKET_SIZE){
		while(Serial.available() != 0){
			Serial.read();
		}
	}
	
	//Enter if data received from base station
	if(_radio.hasAckData()){
		
		SubPacket subData;
		_radio.readData(&subData);
		
		currentSubData[0] = subData.rudderPosition;
		currentSubData[1] = subData.aftDivePosition;
		currentSubData[2] = subData.foreDivePosition;
		currentSubData[3] = (uint8_t)(subData.spoolPosition >> 8);
		currentSubData[4] = (uint8_t)subData.spoolPosition;
		currentSubData[5] = (uint8_t)(subData.ballastPosition >> 8);
		currentSubData[6] = (uint8_t)subData.ballastPosition;
		currentSubData[7] = subData.motorTemp;
		currentSubData[8] = subData.waterSense;
		currentSubData[9] = subData.batteryVoltage;
		currentSubData[10] = 20;//subData.subPacketCheck;
		
		Serial.write(currentSubData, SUB_PACKET_SIZE);
		
	}
	delayMicroseconds(200);
}
