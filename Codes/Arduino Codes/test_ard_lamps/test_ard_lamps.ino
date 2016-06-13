#include <SPI.h>
#include <Ethernet.h>

//button pin
const int BUTTON = 7;

//hue variables
const char hueBridgeIP[] = "192.168.1.162"; // IP found for the Philips Hue bridge
const char hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const int hueBridgePort = 80;

String hueOn[3];
int hueBri[3];
long hueHue[3];
int hueSat[3];
String command;
int randomNum;

byte mac[] = {0x90, 0xA2, 0xDA, 0x0F, 0x48, 0x5D}; // Ethernet shield MAC address
IPAddress ip(192, 168, 1, 201);                    // Arduino IP

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient client;
  
void setup() {
 pinMode(BUTTON, INPUT);
 Serial.begin(9600);
 if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
 }
 delay(10000);
}
void loop() {

    //Serial.println(client.connect(hueBridgeIP, hueBridgePort));
    getHue(3);
    command = "{\"on\": true,\"bri\": 215,\"hue\": 25178,\"sat\":235}";
    setHue(1, command);
    delay(500);
    setHue(2, command);
    delay(500);
    setHue(3, command);
    delay(1000);
    getHue(3);
    command = "{\"on\": false,\"bri\": 215,\"hue\": 25178,\"sat\":235}";
    setHue(3, command);
    delay(500);
    setHue(2, command);
    delay(500);   
    setHue(1, command);
    delay(1000);
}

boolean setHue(int lightNum, String command){
  if (client.connect(hueBridgeIP, hueBridgePort)){
    while (client.connected()){
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

 boolean getHue(int lightNum)
{
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
        hueOn[lightNum-1] = (client.readStringUntil(','));
 
        client.findUntil("\"bri\":", '\0');
        hueBri[lightNum-1] = client.readStringUntil(',').toInt();  
 
        client.findUntil("\"hue\":", '\0');
        hueHue[lightNum-1] = client.readStringUntil(',').toInt(); 

        client.findUntil("\"sat\":", '\0');
        hueSat[lightNum-1] = client.readStringUntil(',').toInt(); 
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
