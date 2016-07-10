/*
  SD card read/write

  This example shows how to read and write data to and from an SD card file
  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4

  created   Nov 2010
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe

  This example code is in the public domain.

*/

#include <SPI.h>
#include <SD.h>
#include<SoftwareSerial.h>

const int debugLED = 13;
const String fileName = "data.txt";

SoftwareSerial xbee(2, 3);

File myFile;

void setup() {

  pinMode(debugLED, OUTPUT);
  xbee.begin(9600);
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {       // pin 4: CS pin - depending on the make of the shield
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  if (SD.exists(fileName)) {
    Serial.print("Do you want to delete previous file? Type n for no or y for yes: ");
    while (Serial.available() == 0);
    char c = Serial.read();
    if (c == 'n' || c == 'N') {
      Serial.println("File kept.");
    }
    else if (c == 'y' || c == 'Y') {
      deleteFile();
      Serial.println("File kept.");
    }
    else Serial.println("Command not known. File kept.");
  }

}

void loop() {

  receivePackage();

  // check Serial
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'd' || c == 'D') {
      deleteFile();
    }
  }

}

void receivePackage() {

  int analogValue = 0, analogHigh, analogLow;
  byte discard;
  boolean received = false;
  float temp = 0;
  byte sender[4];
  String textToWrite;
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
    Serial.print("Sender address: ");
    for (i = 0; i < 4; i++) Serial.print(sender[i]);
    Serial.print("\nValue received: ");
    Serial.print(analogValue);
    Serial.print(" = ");
    Serial.print(temp);
    Serial.println(" ºC");
    textToWrite = "{\"sender:\"" + String(sender[3]) + ",\"time:\"" + String(123456) + ",\"temperature:\"" + String(temp) + "}";
    writeToFile(textToWrite);
    received = false;
  }

}

void writeToFile(String textToWrite) {

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open(fileName, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to " + fileName + "...");
    myFile.println(textToWrite);
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening "+fileName);
  }

}

void readFile() {

  // re-open the file for reading:
  myFile = SD.open(fileName);
  if (myFile) {
    Serial.println(fileName + ":");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening " + fileName);
  }

}

void deleteFile() {

  if (SD.exists(fileName)) {
    SD.remove(fileName);
    if (SD.exists(fileName)) Serial.println("Error deleting " + fileName);
    else Serial.println(fileName + " deleted.");
  }
  else Serial.println(fileName + " does not exist.");

}

