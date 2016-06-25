byte discardByte = 0;
int i = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available() > 0) {
    discardByte = Serial.read();
    if (discardByte == 0x7E) i = 0;
    Serial.print(i);
    //Serial.print(": ");
    Serial.println(discardByte);
    i++;
  }
}
