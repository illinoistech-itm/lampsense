
const int sensorGas = A0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int sensorVal = analogRead(sensorGas);

  Serial.print("Sensor on A0 - ");
  Serial.println(sensorVal);
  /* float voltage = (sensorVal / 1024.0) * 5.0;        // Using a 5V Arduino (Arduino supply voltage, not the supply for sensor)
  Serial.print(", Volts: ");
  Serial.print(voltage);
  Serial.print(", degrees C: ");
  float temperature = (voltage - .5) * 100;
  Serial.println(temperature);
  */
  delay(1000);
}
