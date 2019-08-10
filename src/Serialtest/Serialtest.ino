byte myReceivedData[9];
byte mySentData[10] = {50, 75, 10, 1, 155, 0, 65, 200, 5, 128};
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available() == 9){
    for(int i = 0; i < 9; i++){
      myReceivedData[i] = Serial.read();
    }
    for(int i = 0; i < 10; i++){
      mySentData[i] = random();
      if(mySentData[i] > 125){
        mySentData[i] = 125;
      }
      
    }
    if(mySentData[3] > 2){
      mySentData[3] = 2;
    }
    if(mySentData[5] > 1){
      mySentData[5] = 1;
    }
    Serial.write(mySentData, 10);
  }
  delay(2);
}
