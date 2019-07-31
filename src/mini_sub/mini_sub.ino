/*
Mini-Sub Operation Code.
Receives a data packet from the transmission station. writes the received packet to the serial bus.
Appends data transmitted from the sub over serial to an ack packet,
which is sent to the base station every time a packet from the base station is received.

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

const static uint8_t RADIO_ID = 0;
const static uint8_t DESTINATION_RADIO_ID = 1;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;


//StationPacket - packet received from Base Station
struct StationPacket{
	int8_t driveSetpoint;
	int8_t rudderSetpoint;
	int8_t aftDiveSetpoint;
	int8_t foreDiveSetpoint;
	uint8_t headLightSetpoint;
	uint16_t spoolSetpoint;
	uint16_t ballastSetpoint;
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
	
};

NRFLite _radio;

/*Current Sub Running Data:
Variables holding the latest received operational data from the sub.
Assigned to and transmitted by ack packets.
*/

const uint8_t SUB_PACKET_SIZE = 	9;
byte currentSubData[SUB_PACKET_SIZE];
int8_t rudderPositionCurrent = 		0;
int8_t aftDivePositionCurrent = 	0;
int8_t foreDivePositionCurrent = 	0;
uint16_t spoolPositionCurrent = 	0;
uint16_t ballastPositionCurrent = 	0;
uint8_t motorTempCurrent = 			0;
uint8_t waterSenseCurrent = 		0;

/*Current Station Setpoint Data
Variables holding the latest received setpoint data from the base station.
Written over the serial bus once received
*/

const uint8_t STATION_PACKET_SIZE = 9;
byte currentStationData[STATION_PACKET_SIZE];


void setup(){
	//init serial
    Serial.begin(115200);
	//init radio
    _radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN);
}

void loop(){
	//Enter if data received from sub
	if(Serial.available()>0){
		//wait a few ms for packet to finish transmitting (might be unnessary)
		delay(3);
		for(uint8_t i = 0; i < SUB_PACKET_SIZE; i++){
			currentSubData[i] = Serial.read();
		}
		rudderPositionCurrent = currentSubData[0];
		aftDivePositionCurrent = currentSubData[1];
		foreDivePositionCurrent = currentSubData[2];
		//Handle the bitshifting -> 2 bytes into a uint16_t
		spoolPositionCurrent = currentSubData[3];
		spoolPositionCurrent = spoolPositionCurrent << 8;
		spoolPositionCurrent = spoolPositionCurrent | currentSubData[4];
		//Handle the bitshifting -> 2 bytes into a uint16_t
		ballastPositionCurrent = currentSubData[5];
		ballastPositionCurrent = ballastPositionCurrent << 8;
		ballastPositionCurrent = ballastPositionCurrent | currentSubData[6];
		motorTempCurrent = currentSubData[7];
		waterSenseCurrent = currentSubData[8];
		//At this point, the 'current' vars contain the latest values
	}
	
	//Enter if data received from base station
	if(_radio.hasData()){
		//wait a few ms for packet to finish transmitting (might be unnessary)
		delay(3);
		
		StationPacket stationData;
		_radio.readData(&stationData);
		//Assign the new packet data to the transmit byte array
		currentStationData[0] = stationData.driveSetpoint;
		currentStationData[1] = stationData.rudderSetpoint;
		currentStationData[2] = stationData.aftDiveSetpoint;
		currentStationData[3] = stationData.foreDiveSetpoint;
		currentStationData[4] = stationData.headLightSetpoint;
		currentStationData[5] = stationData.spoolSetpoint >> 8;
		currentStationData[6] = stationData.spoolSetpoint;
		currentStationData[7] = stationData.ballastSetpoint >> 8;
		currentStationData[8] = stationData.ballastSetpoint;
		
		//Write the data to the serial bus:
		Serial.write(currentStationData, STATION_PACKET_SIZE);
		
		//Now create the acknoledge packet with the current Sub Data
		SubPacket subData;
		subData.rudderPosition = rudderPositionCurrent;
		subData.aftDivePosition = aftDivePositionCurrent;
		subData.foreDivePosition = foreDivePositionCurrent;
		subData.spoolPosition = spoolPositionCurrent;
		subData.ballastPosition = ballastPositionCurrent;
		subData.motorTemp = motorTempCurrent;
		subData.waterSense = waterSenseCurrent;
		
		_radio.addAckData(&subData, sizeof(subData));
	}
}
















