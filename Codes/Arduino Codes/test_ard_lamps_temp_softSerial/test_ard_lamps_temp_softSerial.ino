
#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>

//hue variables
const char hueBridgeIP[] = "192.168.1.188"; // IP found for the Philips Hue bridge
const char hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const int hueBridgePort = 80;

const String commandOff = "{\"on\": false}";
const String commandOn  = "{\"on\": true}";

String hueOn[3];
int hueBri[3];
long hueHue[3];
int hueSat[3];
String command;
int randomNum;
const int debugLED = 13;
int analogValue = 0, analogHigh, analogLow;
byte discard;
boolean received = false;
float temp = 0;
int i, c;

byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x49, 0x9A}; // Ethernet shield MAC address
IPAddress ip(192, 168, 1, 254);                    // Arduino IP

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient client;

SoftwareSerial xbee(2, 3); // RX, TX

void setup() {

  pinMode(debugLED, OUTPUT);
  Serial.begin(9600);
  xbee.begin(9600);
  Serial.println("Ethernet.begin initializing");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  Serial.println("Ethernet.begin executado");
  delay(5000);
  Serial.println("Ready for commands");

}


void loop() {

  // make sure everything we need is in the buffer
  if (xbee.available() >= 21) {
    // look for the start byte
    if (xbee.read() == 0x7E) {
      //blink debug LED to indicate when data is received
      digitalWrite(debugLED, HIGH);
      delay(10);
      digitalWrite(debugLED, LOW);
      // read the variables that we're not using out of the buffer
      for (int i = 1; i < 19 ; i++) {
        discard = xbee.read();
      }
      analogHigh = xbee.read();
      analogLow = xbee.read();
      analogValue = analogLow + (analogHigh * 256);
      temp = analogValue / 1023.0 * 1.23;
      temp = (temp - 0.5) / 0.01;
      received = true;
    }
  }
  if (received) {
    Serial.print("Value received: ");
    Serial.print(analogValue);
    Serial.print(" = ");
    Serial.print(temp);
    Serial.println(" ºC");
    tempToLamp(&temp);
    received = false;
  }

}

void tempToLamp(float* temp) {

  if (*temp < 15.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 55000,\"sat\":235}";
    setHue(1, command);
    delay(50);
    setHue(2, command);
    delay(50);
    setHue(3, command);
    delay(50);
  }
  else if (*temp < 25.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 30000,\"sat\":235}";
    setHue(1, command);
    delay(50);
    setHue(2, command);
    delay(50);
    setHue(3, command);
    delay(50);
  }
  else {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 5000,\"sat\":235}";
    setHue(1, command);
    delay(50);
    setHue(2, command);
    delay(50);
    setHue(3, command);
    delay(50);
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
        hueOn[lightNum - 1] = (client.readStringUntil(','));

        client.findUntil("\"bri\":", '\0');
        hueBri[lightNum - 1] = client.readStringUntil(',').toInt();

        client.findUntil("\"hue\":", '\0');
        hueHue[lightNum - 1] = client.readStringUntil(',').toInt();

        client.findUntil("\"sat\":", '\0');
        hueSat[lightNum - 1] = client.readStringUntil(',').toInt();
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


