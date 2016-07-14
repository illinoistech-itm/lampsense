#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>

//hue variables
// const char hueBridgeIP[] = "192.168.1.188"; // IP found for the Philips Hue bridge
const char hueBridgeIP[] = "10.101.2.54"; // IP found for the Philips Hue bridge
const char hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const int hueBridgePort = 80;

const int NUM_LAMPS = 5;
const int BUTTON1 = 7;
const int BUTTON2 = 8;
const unsigned long int INTERVAL = 1000; // 500 ms
const byte SENSOR_TEMP1[] = {0x40, 0xC6, 0x74, 0xF3};
const byte SENSOR_TEMP2[] = {0x40, 0xC6, 0x73, 0xFB};
const byte SENSOR_GAS[] = {0x40, 0xC6, 0x73, 0xE7};
const int MAX_GAS_LEVEL = 500;

String hueOn;
int hueBri;
long hueHue;
int hueSat;
int path[] = {0, 0, 0, 0, 0}; // -1: off, 0: on for normal lighting 1: left path; 2: right path; 3: left and right paths (sum)
int pathLeft[] = {1, 1, 1, 0, 0};  // 1 for lamp being part of such path; 0 for not
int pathRight[] = {1, 0, 0, 1, 1};
int pathUsed[] = {0, 0};
unsigned int color[NUM_LAMPS]; // current color for each lamp
unsigned int colorGas = 0;
String command;
// String commandOn = "{\"on\": true,\"bri\": 215,\"effect\": \"colorloop\",\"alert\": \"select\",\"hue\": 0,\"sat\":0}"; // full command line
String commandOn = "{\"on\": true,\"bri\": 215,\"hue\": 0,\"sat\":0}";
String commandOff = "{\"on\": false,\"bri\": 215,\"hue\": 0,\"sat\":0}";
String commandLeft = "{\"on\": true,\"bri\": 215,\"hue\": 20000,\"sat\":235}";
String commandRight = "{\"on\": true,\"bri\": 215,\"hue\": 50000,\"sat\":235}";
const int debugLED = 13;
int i, c;
unsigned long int previousTimePath = 0xFFFFFFFF, previousTimeGas = 0xFFFFFFFF, currentTime;
float lastValidTemp;
boolean gasDetected = false;


byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x49, 0x9A}; // Ethernet shield MAC address
//IPAddress ip(192, 168, 1, 201);                    // Arduino IP
IPAddress ip(10, 101, 2, 254);                    // Arduino IP

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient client;

SoftwareSerial xbee(2, 3); // RX, TX

void setup() {

  pinMode(debugLED, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  Serial.begin(9600);
  xbee.begin(9600);
  Serial.println("Ethernet.begin initializing");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to configure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.println("Ethernet.begin executado");
  delay(1000);
  Serial.println("Ready for commands");

}


void loop() {


  // Evaluate if button was pressed - Button 1 for left path, button 2 for right path
  if (digitalRead(BUTTON1) == HIGH) {
    deletePath(1, "\n\nPerson arrived at left side");
  } else if (digitalRead(BUTTON2) == HIGH) {
    deletePath(2, "\n\nPerson arrived at right side");
  }

  // Evaluate state of lamps
  if (gasDetected) {
    gasToLamps();
  }
  else {
    for (i = 0; i < NUM_LAMPS; i++) {
      if (path[i] == 3) {
        currentTime = millis();
        if ((previousTimePath + INTERVAL) < currentTime) { // need to fix for millis reseting
          color[i] = 70000 - color[i]; // if color is 20000 turn it to 50000 and vice versa
          Serial.println(color[i]);
          command = "{\"on\": true,\"bri\": 215,\"hue\": " + String(color[i]) + ",\"sat\":235}";
          setHue(i + 1, command);
          previousTimePath = currentTime;
        }
      }
    }
  }

  // Evaluate if input was given via serial
  if (Serial.available() > 0) {
    c = Serial.read();
    if (c == 't' || c == 'T') {
      showTemp();
    }
    if (c == 'L' || c == 'l') {
      //getPreviousState();
      addPath(1, "Going to left <--");
    }
    else if (c == 'R' || c == 'r') {
      //getPreviousState();
      addPath(2, "Going to right -->");
    }
    else if (c == 'N' || c == 'n') {
      //getPreviousState();
      setAllLamps(0, "Turning all on");    // Second parameter to 0 for all lamps on
    }
    else if (c == 'F' || c == 'f') {
      //getPreviousState();
      setAllLamps(-1, "Turning all off");
    }
  }

  //Evaluate if input was given via Xbee serial port
  getTemp();

}


void getTemp() {

  byte discard, analogHigh, analogLow;
  int analogValue = 0;
  int sender[4];
  float temp = 0;
  boolean received = false;

  // make sure everything we need is in the buffer
  if (xbee.available() >= 21) {
    // look for the start byte
    if (xbee.read() == 0x7E) {
      // read the variables that we're not using out of the buffer
      for (i = 1; i < 8 ; i++) discard = xbee.read();
      // address of sender (bytes from 8 to 11)
      for (i = 8; i < 12; i++) sender[i] = xbee.read();
      for (i = 12; i < 19; i++) discard = xbee.read();
      analogHigh = xbee.read();
      analogLow = xbee.read();
      analogValue = analogLow + (analogHigh * 256);
      analyzeMessage(sender, &analogValue);
    }
  }

}

void analyzeMessage(int sender[], int* analogValue) {

  float temp = 0;
  if (sender[3] == SENSOR_TEMP1[3] || sender[3] == SENSOR_TEMP2[3]) { // SENSOR_TEMP1
    temp = convertTemp(analogValue);
    if (temp > 0.0 || temp < 50.0) {
      lastValidTemp = temp;
    }
  } else if (sender[3] == SENSOR_GAS[3]) {
    analyzeGasLevel(analogValue);
  }

}

void analyzeGasLevel(int* analogValue) {

  if (*analogValue > MAX_GAS_LEVEL) {
    gasDetected = true;
  }
  else gasDetected = false;

}

float convertTemp(int* analogValue) {

  float temp = 0;
  temp = *analogValue / 1023.0 * 1.23;
  temp = (temp - 0.5) / 0.01;

  return temp;

}

void showTemp() {

  Serial.print("Temperature: ");
  Serial.print(lastValidTemp);
  Serial.println(" ºC");
  tempToLamp(&lastValidTemp);
  delay(3000);
  setAllLamps(0, commandOn);
  pathUsed[0] = pathUsed[1] = 0;

}

void tempToLamp(float* temp) {

  if (*temp < 15.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 55000,\"sat\":235}";
    setAllLamps(0, command);
  }
  else if (*temp < 25.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 30000,\"sat\":235}";
    setAllLamps(0, command);
  }
  else {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 5000,\"sat\":235}";
    setAllLamps(0, command);
  }

}


void gasToLamps() {

  currentTime = millis();
  if ((previousTimeGas + INTERVAL) < currentTime) { // need to fix for millis reseting
    colorGas = 5000 - colorGas; // if color is 5000 turn it to 0 and vice versa
    Serial.println(colorGas);
    command = "{\"on\": true,\"bri\": 215,\"hue\": " + String(colorGas) + ",\"sat\":235}";
    setAllLamps(-2, command);
    previousTimeGas = currentTime;
  }
  command = "{\"on\": true,\"bri\": 215,\"hue\": 5000,\"sat\":235}";
  setAllLamps(0, command);

}

void setAllLamps(int numPath, String message) {  // numPath: -1 for all off, 0 for all on (normal), 1 for left, 2 for right

  Serial.println(message);
  if (numPath == -1) {
    command = commandOff;
    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = -1;
      delay(50);
    }
  }
  else if (numPath == 0) {
    command = commandOn;
    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 0;
      delay(50);
    }
  }
  else if (numPath == 1) {
    command = commandLeft;
    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 1;
      delay(50);
    }
  }
  else if (numPath == 2) {
    command = commandRight;
    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 2;
      delay(50);
    }
  }

}

void addPath(int addedPath, String message) {  // addedPath: 1 for left, 2 for right

  Serial.println(message);
  if (addedPath == 1) { // left case
    if (pathUsed[0] == 1) Serial.println("Error: left path is already being used");
    else {
      pathUsed[0] = 1;    // change - left path being used
      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathLeft[j - 1] == 1) {
          setHue(j, commandLeft);
          path[j - 1] = path[j - 1] + addedPath;
          color[j - 1] = 20000;
          delay(50);
        }
      }
    }
  }
  else if (addedPath == 2) { // right case
    if (pathUsed[1] == 1) Serial.println("Error: right path is already being used");
    else {
      pathUsed[1] = 1;    // change - right path being used
      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathRight[j - 1] == 1) {
          setHue(j, commandRight);
          path[j - 1] = path[j - 1] + addedPath;
          color[j - 1] = 50000;
          delay(50);
        }
      }
    }
  }
  else Serial.println("Error in addPath function");

}

void deletePath(int modifiedPath, String message) {  // modifiedPath: 1 for left, 2 for right

  Serial.println(message);
  if (modifiedPath == 1) { // left case
    if (pathUsed[0] == 0) Serial.println("Error: left path is not being used");
    else {
      pathUsed[0] = 0;
      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathLeft[j - 1] == 1) {
          if (path[j - 1] == 3) setHue(j, commandRight);
          else setHue(j, commandOn);
          path[j - 1] = path[j - 1] - modifiedPath;
          delay(50);
        }
      }
    }
  }
  else if (modifiedPath == 2) { // right case
    if (pathUsed[1] == 0) Serial.println("Error: right path is not being used");
    else {
      pathUsed[1] = 0;
      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathRight[j - 1] == 1) {
          if (path[j - 1] == 3) setHue(j, commandLeft);
          else setHue(j, commandOn);
          path[j - 1] = path[j - 1] - modifiedPath;
          delay(50);
        }
      }
    }
  }
  else Serial.println("Error in deletePath function");

}

boolean setHue(int lightNum, String command) {

  if (client.connect(hueBridgeIP, hueBridgePort)) {
    if (client.connected()) {
      client.print("PUT /api/");
      client.print(hueUsername);
      client.print("/lights/");
      client.print(lightNum);
      client.println("/state HTTP/1.1");
      client.println("keep-alive");
      client.print("Host: ");
      client.println(hueBridgeIP);
      client.print("Content-Length: ");
      client.print(command.length());
      client.println("Content-Type: text/plain;charset=UTF-8");
      client.println();
      client.println(command);
      Serial.print("Sent command for light #");
      Serial.println(lightNum);
    }
    client.stop();
    return true;
  }
  else {
    Serial.println("setHue() - Command failed");
    return false;
  }

}

/*boolean getHue(int lightNum) {

  if (client.connect(hueBridgeIP, hueBridgePort))
  {
    client.print("GET /api/");
    client.print(hueUsername);
    client.print("/lights/");
    client.print(lightNum);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(hueBridgeIP);
    client.println("Content-type: application/json");
    client.println("keep-alive");
    client.println();
    while (client.connected())
    {
      if (client.available())
      {
        //read the current bri, hue, sat, and whether the light is on or off
        client.findUntil("\"on\":", '\0');
        hueOn = (client.readStringUntil(','));

        client.findUntil("\"bri\":", '\0');
        hueBri = client.readStringUntil(',').toInt();

        client.findUntil("\"hue\":", '\0');
        hueHue = client.readStringUntil(',').toInt();

        client.findUntil("\"sat\":", '\0');
        hueSat = client.readStringUntil(',').toInt();
        break;
      }
    }
    client.stop();
    return true;
  }
  else
    Serial.println("getHue() - Command failed");
  return false;

  }*/

/*void getPreviousState() {

  Serial.println("\n\nReading current state");
  getHue(1);
  delay(10);
  getMessagesDebug("Light 1 - on: ");
  getHue(2);
  delay(10);
  getMessagesDebug("Light 2 - on: ");
  getHue(3);
  delay(10);
  getMessagesDebug("Light 3 - on: ");

  }

  void getMessagesDebug(String message) {

  Serial.print(message);
  Serial.print(hueOn);
  Serial.print(", hueBri: ");
  Serial.print(hueBri);
  Serial.print(", hueHue: ");
  Serial.print(hueHue);
  Serial.print(", hueSat: ");
  Serial.print(hueSat);
  Serial.println();

  }*/



