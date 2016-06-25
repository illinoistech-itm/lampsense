const int sensor1Pin = A0;
const int sensor2Pin = A1;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int sensorVal1 = analogRead(sensor1Pin);
  int sensorVal2 = analogRead(sensor2Pin);

  Serial.print("Sensor on A0 - ");
  Serial.print(sensorVal1);
  float voltage = (sensorVal1 / 1024.0) * 5.0;        // Using a 5V Arduino (Arduino supply voltage, not the supply for sensor)
  Serial.print(", Volts: ");
  Serial.print(voltage);
  Serial.print(", degrees C: ");
  float temperature = (voltage - .5) * 100;
  Serial.println(temperature);

  /*Serial.print("Sensor on A1 - ");
  Serial.print(sensorVal2);
  voltage = (sensorVal2 / 1024.0) * 5.0;
  Serial.print(", Volts: ");
  Serial.print(voltage);
  Serial.print(", degrees C: ");
  temperature = (voltage - .5) * 100;
  Serial.println(temperature);*/

  delay(1000);
}
