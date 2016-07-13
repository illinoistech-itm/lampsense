
#include<SoftwareSerial.h>

const int debugLED = 13;
int analogValue = 0, analogHigh, analogLow;
byte discard;
boolean received = false;
float temp = 0;
byte sender[4];

SoftwareSerial xbee(2, 3);

void setup() {
  pinMode(debugLED, OUTPUT);
  Serial.begin(9600);
  xbee.begin(9600);
}

void loop() {
  int i;
  // make sure everything we need is in the buffer
  if (xbee.available() >= 21) {
    // look for the start byte
    if (xbee.read() == 0x7E) {
      //blink debug LED to indicate when data is received
      digitalWrite(debugLED, HIGH);
      delay(10);
      digitalWrite(debugLED, LOW);
      // read the variables that we're not using out of the buffer
      for (i = 1; i < 8 ; i++) discard = xbee.read();
      // address of sender (bytes from 8 to 11)
      for (i = 0; i < 4; i++) sender[i] = xbee.read();
      for (i = 12; i < 19; i++) discard = xbee.read();
      analogHigh = xbee.read();
      analogLow = xbee.read();
      analogValue = analogLow + (analogHigh * 256);
      temp = analogValue / 1023.0 * 1.23;
      temp = (temp - 0.5) / 0.01;
      received = true;
    }
  }
  if (received) {
    Serial.print("\nSender address: ");
    for(int i=0; i<4; i++){ Serial.print(sender[i], HEX); Serial.print(" "); }
    Serial.print("\nValue received: ");
    Serial.print(analogValue);
    Serial.print(" = ");
    Serial.print(temp);
    Serial.println(" ºC");
    received = false;
  }
}


