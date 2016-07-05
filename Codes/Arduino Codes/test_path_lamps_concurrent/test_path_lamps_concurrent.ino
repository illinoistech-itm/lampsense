#include <SPI.h>
#include <Ethernet.h>

//hue variables
const char hueBridgeIP[] = "192.168.1.188"; // IP found for the Philips Hue bridge
const char hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const int hueBridgePort = 80;

const int pinButton = 7;
const unsigned long int interval = 500; // 500 ms

String hueOn;
int hueBri;
long hueHue;
int hueSat;
int path[] = {0, 0, 0}; // 0: used just for lighting; 1: left path; 2: right path; 3: both paths (sum)
unsigned int color[3]; // current color for each lamp
String command;
// String commandOn = "{\"on\": true,\"bri\": 215,\"effect\": \"colorloop\",\"alert\": \"select\",\"hue\": 0,\"sat\":0}"; // full command line
String commandOn = "{\"on\": true,\"bri\": 215,\"hue\": 0,\"sat\":0}";
String commandOff = "{\"on\": false}";
const int debugLED = 13;
int i, c;
unsigned long int previousTime = 0xFFFFFFFF, currentTime;


byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x49, 0x9A}; // Ethernet shield MAC address
IPAddress ip(192, 168, 1, 201);                    // Arduino IP

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient client;

void setup() {

  pinMode(debugLED, OUTPUT);
  pinMode(pinButton, INPUT);
  Serial.begin(9600);
  Serial.println("Ethernet.begin initializing");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.println("Ethernet.begin executado");
  delay(1000);
  Serial.println("Ready for commands");

}


void loop() {

  // Evaluate state of lamps
  for (i = 0; i < 3; i++) {
    if (path[i] == 3) {
      currentTime = millis();
      if ((previousTime + interval) < currentTime) { // need to fix for millis reseting
        color[i] = 70000 + color[i]; // if color is 20000 turn it to 50000 and vice versa
        command = "{\"on\": true,\"bri\": 215,\"hue\": " + String(color[i]) + ",\"sat\":235}";
        setHue(i + 1, command);
        previousTime = currentTime;
      }
    }
  }

  // Evaluate if button was pressed
  if (digitalRead(pinButton) == HIGH) {
    command = "{\"on\": " + hueOn + ",\"bri\": " + hueBri + ",\"hue\": " + hueHue + ",\"sat\": " + hueSat + "}";
    set3Lamps(command, 0, 0, "\n\nRestauring previous state of lamp 3 for all lamps");
  }

  // Evaluate if input was given via serial
  if (Serial.available() > 0) {
    c = Serial.read();
    if (c == 'L' || c == 'l') {
      getPreviousState();
      command = "{\"on\": true,\"bri\": 215,\"hue\": 25178,\"sat\":235}";
      set3Lamps(command, 2, 1, "Going to left <--");
    }
    else if (c == 'R' || c == 'r') {
      getPreviousState();
      command = "{\"on\": true,\"bri\": 215,\"hue\": 50000,\"sat\":235}";
      set3Lamps(command, 3, 2, "Going to right -->");
    }
    else if (c == 'N' || c == 'n') {
      getPreviousState();
      set3Lamps(commandOn, 0, 0, "Turning all on"); // Second parameter to 0 for no difference between lamps
    }
    else if (c == 'F' || c == 'f') {
      getPreviousState();
      set3Lamps(commandOff, 0, 0, "Turning all off");
    }
  }

}


void set3Lamps(String command, int lampOff, int numPath, String message) {

  Serial.println(message);
  if (lampOff == 0) {
    for (int j = 1; j < 4; j++) {
      setHue(j, command);
      path[j - 1] = 0;
      delay(50);
    }
  }
  else {
    for (int j = 1; j < 4; j++) {
      if (j == lampOff) {
        setHue(j, commandOff);
        path[j - 1] = 0;
        delay(50);
      } else {
        setHue(j, command);
        path[j - 1] = (path[j-1]+numPath)%4;
        delay(50);
      }
    }
  }

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

boolean getHue(int lightNum) {

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

}

void getPreviousState() {

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

}



