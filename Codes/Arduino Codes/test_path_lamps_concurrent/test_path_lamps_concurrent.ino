#include <SPI.h>
#include <Ethernet.h>

//hue variables
const char hueBridgeIP[] = "192.168.1.188"; // IP found for the Philips Hue bridge
const char hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const int hueBridgePort = 80;

const int pinButton = 7;

String hueOn;
int hueBri;
long hueHue;
int hueSat;
String command;
// String commandOn = "{\"on\": true,\"bri\": 215,\"effect\": \"colorloop\",\"alert\": \"select\",\"hue\": 0,\"sat\":0}";
String commandOn = "{\"on\": true,\"bri\": 215,\"hue\": 0,\"sat\":0}";
String commandOff = "{\"on\": false}";
const int debugLED = 13;
int i, c;

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

  if (digitalRead(pinButton) == HIGH) {
    command = "{\"on\": " + hueOn + ",\"bri\": " + hueBri + ",\"hue\": " + hueHue + ",\"sat\": " + hueSat + "}";
    set3Lamps(command, 0, "\n\nRestauring previous state of lamp 3 for all lamps");
  }
  if (Serial.available() > 0) {
    c = Serial.read();
    if (c == 'L' || c == 'l') {
      getPreviousState();
      command = "{\"on\": true,\"bri\": 215,\"hue\": 25178,\"sat\":235}";
      set3Lamps(command, 2, "Going to left <--");
    }
    else if (c == 'R' || c == 'r') {
      getPreviousState();
      command = "{\"on\": true,\"bri\": 215,\"hue\": 50000,\"sat\":235}";
      set3Lamps(command, 3, "Going to right -->");
    }
    else if (c == 'N' || c == 'n') {
      getPreviousState();
      set3Lamps(commandOn, 0, "Turning all on"); // Second parameter to 0 for no difference between lamps
    }
    else if (c == 'F' || c == 'f') {
      getPreviousState();
      set3Lamps(commandOff, 0, "Turning all off");
    }
  }

}

void set3Lamps(String command, int lampOff, String message) {

  Serial.println(message);
  if (lampOff == 0) {
    for (int j = 1; j < 4; j++) {
      setHue(j, command);
      delay(50);
    }
  }
  else {
    for (int j = 1; j < 4; j++) {
      if (j == lampOff) {
        setHue(j, commandOff);
        delay(50);
      } else {
        setHue(j, command);
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



