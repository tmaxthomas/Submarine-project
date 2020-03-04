void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(52, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Water Sensor: ");
  Serial.println(analogRead(5));
  Serial.print("Battery Voltage: ");
  Serial.println(analogRead(15));

  digitalWrite(52, HIGH);
  Serial.print("Motor Temp: ");
  Serial.println(analogRead(0));
  digitalWrite(52, LOW);
  
  Serial.println();
  delay(500);
  
}
