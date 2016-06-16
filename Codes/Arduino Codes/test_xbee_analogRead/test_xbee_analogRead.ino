
const int debugLED = 13;
int analogValue = 0, analogHigh, analogLow;
byte discard;
boolean received = false;
float temp = 0;

void setup() {
  pinMode(debugLED, OUTPUT);
  Serial.begin(9600);


}

void loop() {
  // make sure everything we need is in the buffer
  if (Serial.available() >= 21) {
    // look for the start byte
    if (Serial.read() == 0x7E) {
      //blink debug LED to indicate when data is received
      digitalWrite(debugLED, HIGH);
      delay(10);
      digitalWrite(debugLED, LOW);
      // read the variables that we're not using out of the buffer
      for (int i = 0; i < 18; i++) {
        discard = Serial.read();
      }
      analogHigh = Serial.read();
      analogLow = Serial.read();
      analogValue = analogLow + (analogHigh * 256);
      temp = analogValue/1023.*1.23;
      temp -= 0.5;
      temp *= 100;
      received = true;
    }
  }
  if (received) {
    Serial.print("Value received: ");
    Serial.print(analogValue);
    Serial.print(" = ");
    Serial.print(temp);
    Serial.println(" ºC");
    received = false;
  }
}


