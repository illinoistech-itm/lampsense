/* ==============================================================================

    Project: Intelligent Lights and Sensor Node using XBees

    Description:

    Circuit:

    Created:
    By Andreive Giovanini Silva, Breno Soares Martis, Pedro Henrique Gualdi

    URL: github

  ============================================================================== */


/* Libraries */
#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>


/* Function declarations */

/* Ethernet variables */

// Arduino Parameters
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // MAC address from Ethernet shield sticker under board
IPAddress ip(192, 168, 1, 252); // IP address, may need to change depending on network
EthernetServer server(80); // create a server at port 80

// Database connection variables
IPAddress serverDB(192, 168, 1, 153);
EthernetClient clientDB;

/* Macros */
#define BUTTON1    7
#define BUTTON2    8
#define DEBUG_LED  13
#define NUM_LAMPS  6
#define LAMP_OFF   0
#define LAMP_ON    1
#define LAMP_LEFT  2
#define LAMP_RIGHT 4
#define LAMP_BOTH  6


/* Constant variables */
// Philips Hue variables
const char    hueBridgeIP[] = "192.168.1.188"; // IP found for the Philips Hue bridge
const char    hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const uint8_t hueBridgePort = 80;

// Constant identifiers
const byte SENSOR_TEMP1[] = {0x40, 0xC6, 0x74, 0xF3};
const byte SENSOR_TEMP2[] = {0x40, 0xC6, 0x73, 0xFB};
const byte SENSOR_GAS[]   = {0x40, 0xC6, 0x73, 0xE7};

// Constant parameters
const uint64_t INTERVAL = 1000, INTERVAL_DB = 5000;
const uint16_t MAX_GAS_LEVEL = 500;

// Constant strings
// String commandOn = "{\"on\": true,\"bri\": 215,\"effect\": \"colorloop\",\"alert\": \"select\",\"hue\": 0,\"sat\":0}"; // full command line
const String commandOn    = "{\"on\": true,\"bri\": 215,\"hue\": 0,\"sat\":0}";
const String commandOff   = "{\"on\": false,\"bri\": 215,\"hue\": 0,\"sat\":0}";
const String commandLeft  = "{\"on\": true,\"bri\": 215,\"hue\": 20000,\"sat\":235}";
const String commandRight = "{\"on\": true,\"bri\": 215,\"hue\": 50000,\"sat\":235}";

// Constants for paths: 1 for lamp being part of such path, 0 for not
const uint8_t pathLeft[NUM_LAMPS]  = {1, 0, 0, 1, 1, 1};
const uint8_t pathRight[NUM_LAMPS] = {1, 1, 1, 0, 0, 0};


/* Global variables */
int8_t   path[NUM_LAMPS] = {0, 0, 0, 0, 0, 0}; // 0: off, 1: on for normal lighting, 2: left path; 4: right path; 6: left and right paths (sum)
boolean  pathUsed[]      = {false, false};             // Length equal to number of different paths
uint8_t  i;
uint32_t color[NUM_LAMPS];                     // current color for each lamp
uint32_t colorGas = 0;
uint64_t previousTimePath = 0xFFFFFFFF, previousTimeGas = 0xFFFFFFFF, currentTime;
float    lastValidTemp;
char     cmd;
String command;
boolean gasDetected = false;


/* Initialize the Ethernet server library */
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient clientB;


/* Initialize SoftwareSerial ports */
SoftwareSerial xbee(12, 13); // RX, TX


/* ==============================================================================
    Setup function
  ============================================================================== */
void setup() {

  // Initialize pins
  pinMode(DEBUG_LED, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  // Initialize Serial services
  Serial.begin(9600);
  xbee.begin(9600);

  // Initialize Ethernet server
  Serial.println("Ethernet.begin initializing");
  Ethernet.begin(mac, ip); // initialize Ethernet device
  server.begin(); // start to listen for clients
  Serial.println("Ethernet.begin executado");
  delay(1000);
  Serial.println("Ready for commands");

}


/* ==============================================================================
    Loop main function
  ============================================================================== */
void loop() {

  String httpReq; // stores the HTTP request
  EthernetClient client = server.available(); // try to get client

  if (client) { // got client?
    boolean currentLineIsBlank = true;

    while (client.connected()) {
      if (client.available()) { // client data available to read
        char c = client.read(); // read 1 byte (character) from client
        httpReq += c; // save the HTTP request 1 char at a time

        if (httpReq.indexOf("favicon.ico") > -1) { // prevent Google Chrome of sending the request twice
          httpReq = "";
          break;
        }

        // last line of client request is blank and ends with \n
        // respond to client only after last line received
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          sendHtmlPage(client, httpReq); // send web page
          Serial.println();
          Serial.print(httpReq);
          Serial.println();
          processSelection(httpReq);
          httpReq = ""; // finished with request, empty string
          break;
        }
        // every line of text received from the client ends with \r\n
        if (c == '\n') {
          // last character on line of received text
          // starting new line with next character read
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // a text character was received from client
          currentLineIsBlank = false;
        }
      } // end if (client.available())
    } // end while (client.connected())
    delay(1); // give the web browser time to receive the data
    client.stop(); // close the connection
  } // end if (client)

  // Evaluate if button was pressed - Button 1 for left path, button 2 for right path
  if (digitalRead(BUTTON1) == HIGH) {
    deletePath(LAMP_LEFT, "\n\nPerson arrived at left side.");
  } else if (digitalRead(BUTTON2) == HIGH) {
    deletePath(LAMP_RIGHT, "\n\nPerson arrived at right side.");
  }

  // Evaluate state of lamps
  if (gasDetected) {
    gasToLamps();
  }
  else {
    for (i = 0; i < NUM_LAMPS; i++) {
      if (path[i] == LAMP_BOTH) {
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
    cmd = Serial.read();

    if (cmd == 't' || cmd == 'T') {
      showTemp();
    }

    if (cmd == 'L' || cmd == 'l') {
      //getPreviousState();
      addPath(LAMP_LEFT, "Going to left <--");
    } else if (cmd == 'R' || cmd == 'r') {
      //getPreviousState();
      addPath(LAMP_RIGHT, "Going to right -->");
    } else if (cmd == 'N' || cmd == 'n') {
      //getPreviousState();
      setAllLamps(LAMP_ON, "Turning all on");
    } else if (cmd == 'F' || cmd == 'f') {
      //getPreviousState();
      setAllLamps(LAMP_OFF, "Turning all off");
    }
  }

  // Evaluate if input was given via Xbee serial port
  getTemp();

  // Send the temperature via GET every X (INTERVAL_DB) milliseconds
  currentTime = millis();
  if ((previousTimePath + INTERVAL_DB) < currentTime) {
    sendTempViaGet();
    previousTimePath = currentTime;
  }
}

/* ==============================================================================
    Auxiliary functions
  ============================================================================== */
// Connect and send an HTTP request to the database
void connect() {
  if (clientDB.connect(serverDB, 8081)) {
    Serial.println("Connection to the DB server successful.");
  } else {
    Serial.println("Connection to the DB server failed.");
  }
}

// Send the last valid temperature via HTTP request (GET method)
void sendTempViaGet() {
  Serial.println("Attempting to send an HTTP request...");

  if (clientDB.connected()) {
    Serial.println("Sending HTTP request...");

    clientDB.println("GET /insertTemperature?room=RoomTest&temperature=TempTest&datetime=DateTest HTTP/1.0");
    clientDB.println("HOST: 192.168.1.153:8081");
    clientDB.println();

    Serial.print("HTTP request sent.");
  } else {
    connect();
  }
}

// Operate the lamps according to the button pressed
void processSelection(String httpReq) {
  if (httpReq.indexOf("GET /?command=left") > -1) {
    addPath(LAMP_LEFT, "Going to left <--");
  } else if (httpReq.indexOf("GET /?command=right") > -1) {
    addPath(LAMP_RIGHT, "Going to right -->");
  } else if (httpReq.indexOf("GET /?command=on") > -1) {
    setAllLamps(LAMP_ON, "Turning all on"); // Second parameter to 0 for all lamps on
  } else if (httpReq.indexOf("GET /?command=off") > -1) {
    setAllLamps(LAMP_OFF, "Turning all off");
  } else if (httpReq.indexOf("GET /?command=temp") > -1) {
    showTemp();
  } else if (httpReq.indexOf("GET /?command=arrivedLeft") > -1) {
    deletePath(LAMP_LEFT, "\n\nPerson arrived at left side.");
  } else if (httpReq.indexOf("GET /?command=arrivedRight") > -1) {
    deletePath(LAMP_RIGHT, "\n\nPerson arrived at right side.");
  } else {
    Serial.println("Error in processSelection function.");
  }
}

void sendHtmlPage(EthernetClient client, String httpReq) {
  client.println(
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title></title>"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<!-- Latest compiled and minified CSS -->"
    "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\" integrity=\"sha384-1q8mTJOASx8j1Au+a5WDVnPi2lkFfwwEAa8hDDdjZlpLegxhjVME1fgjWPGmkzs7\" crossorigin=\"anonymous\">"
    "<!-- Optional theme -->"
    "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap-theme.min.css\" integrity=\"sha384-fLW2N01lMqjakBkx3l/M9EahuwpSfeNvV63J5ezn3uZzapT0u7EYsXMjQV+0En5r\" crossorigin=\"anonymous\">"
    "<script type=\"text/javascript\">"
    "function getUrlVars() {"
    "var vars = {};"
    "var parts = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function(m,key,value) {"
    "vars[key] = value;"
    "});"
    "return vars;"
    "}"
    "</script>"
    "</head>"
    "<body>"
    "<div class=\"container\">"
  );

  client.println(
    "<script type=\"text/javascript\">"
    "var command = getUrlVars()['command'];"
    "console.log(command);"
    "if (command != undefined) {"
    "document.write(`<div class=\"alert alert-info fade in\">"
    "<a href=\"#\" class=\"close\" data-dismiss=\"alert\" aria-label=\"close\">&times;</a>`);"
    "if (command == 'left') {"
    "document.write('<strong>Processing...</strong> Lightning the path to the left.');"
    "} else if (command == 'right') {"
    "document.write('<strong>Processing...</strong> Lightning the path to the right.');"
    "} else if (command == 'on') {"
    "document.write('<strong>Processing...</strong> Turning all the lights on.');"
    "} else if (command == 'off') {"
    "document.write('<strong>Processing...</strong> Turning all the lights off.');"
    "} else if (command == 'temp') {"
    "document.write('<strong>Processing...</strong> Coloring all the lights according to the temperature.');"
    "} else if (command == 'arrivedLeft') {"
    "document.write('<strong>Processing...</strong> Resetting all the lights in the path to the left.');"
    "} else if (command == 'arrivedRight') {"
    "document.write('<strong>Processing...</strong> Resetting all the lights in the path to the right.');"
    "}"
    "document.write('</div>');"
    "}"
    "</script>"
  );

  client.println(
    "<div class=\"jumbotron\">"
    "<h1>Arduino + Philips Hue</h1> "
    "<p>Click on the buttons to operate the lights.</p> "
    "</div>"
    "<div class=\"btn-group\">"
    "<a class=\"btn btn-primary btn-lg\" href=\"?command=left\">"
    "<span class=\"glyphicon glyphicon-arrow-left\" aria-hidden=\"true\"></span> Left"
    "</a>"
    "<a class=\"btn btn-primary btn-lg\" href=\"?command=right\">"
    "<span class=\"glyphicon glyphicon-arrow-right\" aria-hidden=\"true\"></span> Right"
    "</a>"
    "<a class=\"btn btn-primary btn-lg\" href=\"?command=on\">"
    "<span class=\"glyphicon glyphicon-lamp\" aria-hidden=\"true\"></span> Turn All On"
    "</a>"
    "<a class=\"btn btn-primary btn-lg\" href=\"?command=off\">"
    "<span class=\"glyphicon glyphicon-off\" aria-hidden=\"true\"></span> Turn All Off"
    "</a>"
    "<a class=\"btn btn-primary btn-lg\" href=\"?command=temp\">"
    "<span class=\"glyphicon glyphicon-fire\" aria-hidden=\"true\"></span> Temperature"
  );

  if (httpReq.indexOf("GET /?command=temp") > -1) {
    client.print(" <span class=\"badge\">");
    client.print(lastValidTemp);
    client.println(" °C</span>");
  }

  client.println(
    "</a>"
    "<a class=\"btn btn-primary btn-lg\" href=\"?command=arrivedLeft\">"
    "<span class=\"glyphicon glyphicon-arrow-left\" aria-hidden=\"true\"></span> Arrived Left"
    "</a>"
    "<a class=\"btn btn-primary btn-lg\" href=\"?command=arrivedRight\">"
    "<span class=\"glyphicon glyphicon-arrow-right\" aria-hidden=\"true\"></span> Arrived Right"
    "</a>"
    "</div>"
    "</div>"
    "<!-- Latest (compatible) compiled and minified jQuery -->"
    "<script src=\"https://code.jquery.com/jquery-2.2.4.min.js\" integrity=\"sha256-BbhdlvQf/xTY9gja0Dq3HiwQF8LaCRTXxZKRutelT44=\" crossorigin=\"anonymous\"></script>"
    "<!-- Latest compiled and minified JavaScript -->"
    "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\" integrity=\"sha384-0mSbJDEHialfmuBBQP6A4Qrprq5OVfW37PRR3j5ELqxss1yVqOtnepnHVP9aJ7xS\" crossorigin=\"anonymous\"></script>"
    "</body>"
    "</html>"
  );
}
void getTemp() {

  byte discard, analogHigh, analogLow;
  uint16_t analogValue = 0;
  uint8_t sender[4];
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

void analyzeMessage(uint8_t sender[], uint16_t* analogValue) {

  float temp = 0;
  if (sender[3] == SENSOR_TEMP1[3] || sender[3] == SENSOR_TEMP2[3]) { // In this case, only last byte of sender is sufficient to distinguish
    temp = convertTemp(analogValue);
    if (temp > 0.0 || temp < 50.0) {        // discard "wrong" values, i.e. values uncommon. Lower limit may be adjusted
      lastValidTemp = temp;
    }
  } else if (sender[3] == SENSOR_GAS[3]) {
    analyzeGasLevel(analogValue);
  }

}

void analyzeGasLevel(uint16_t* analogValue) {

  if (*analogValue > MAX_GAS_LEVEL) {
    gasDetected = true;
  }
  else gasDetected = false;

}

float convertTemp(uint16_t* analogValue) {

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

// Color the lamps according to the temperature
void tempToLamp(float* temp) {

  if (*temp < 15.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 55000,\"sat\":235}";
    setAllLamps(-2, command);
  } else if (*temp < 20.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 42000,\"sat\":235}";
    setAllLamps(-2, command);
  } else if (*temp < 25.0) {
    command = "{\"on\": true,\"bri\": 100,\"hue\": 35000,\"sat\":255}";
    setAllLamps(-2, command);
  } else if (*temp < 30.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 10000,\"sat\":235}";
    setAllLamps(-2, command);
  } else {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 5000,\"sat\":235}";
    setAllLamps(-2, command);
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

void setAllLamps(uint8_t numPath, String message) {  // numPath: 0 for all off, 1 for all on (normal), 2 for left, 4 for right
  Serial.println(message);

  if (numPath == LAMP_OFF) {
    command = commandOff;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = -1;
      delay(50);
    }
    pathUsed[0] = pathUsed[1] = false;
  } else if (numPath == LAMP_ON) {
    command = commandOn;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 0;
      delay(50);
    }
    pathUsed[0] = pathUsed[1] = false;
  } else if (numPath == LAMP_LEFT) {
    command = commandLeft;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 1;
      delay(50);
    }
    pathUsed[0] = true;
    pathUsed[1] = false;
  } else if (numPath == LAMP_RIGHT) {
    command = commandRight;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 2;
      delay(50);
    }
    pathUsed[0] = 0 ;
    pathUsed[1] = true;
  } else {
    command = message;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 0;
      delay(50);
    }
    pathUsed[0] = pathUsed[1] = false;
  }
}

void addPath(uint8_t addedPath, String message) {  // addedPath: 1 for left, 2 for right

  uint8_t j;
  Serial.println(message);

  if (addedPath == LAMP_LEFT) { // left case
    if (pathUsed[0] == true) {
      Serial.println("Error: left path is already being used");
    } else {
      pathUsed[0] = true; // change - left path being used

      for (j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathLeft[j - 1] == 1) {
          setHue(j, commandLeft);
          path[j - 1] = path[j - 1] + addedPath;
          color[j - 1] = 20000;
          delay(50);
        }
      }
    }
  } else if (addedPath == LAMP_RIGHT) { // right case
    if (pathUsed[1] == true) {
      Serial.println("Error: right path is already being used");
    } else {
      pathUsed[1] = true; // change - right path being used

      for (j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathRight[j - 1] == 1) {
          setHue(j, commandRight);
          path[j - 1] = path[j - 1] + addedPath;
          color[j - 1] = 50000;
          delay(50);
        }
      }
    }
  } else {
    Serial.println("Error in addPath function");
  }

}

void deletePath(int modifiedPath, String message) { // modifiedPath: 1 for left, 2 for right

  uint8_t j;

  Serial.println(message);

  if (modifiedPath == LAMP_LEFT) { // left case
    if (pathUsed[0] == false) {
      Serial.println("Error: left path is not being used");
    } else {
      pathUsed[0] = false;

      for (j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathLeft[j - 1] == true) {
          if (path[j - 1] == LAMP_BOTH) {
            setHue(j, commandRight);
          } else {
            setHue(j, commandOn);
          }

          path[j - 1] = path[j - 1] - modifiedPath;
          delay(50);
        }
      }
    }
  } else if (modifiedPath == LAMP_RIGHT) { // right case
    if (pathUsed[1] == false) {
      Serial.println("Error: right path is not being used");
    } else {
      pathUsed[1] = false;

      for (j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathRight[j - 1] == true) {
          if (path[j - 1] == LAMP_BOTH) {
            setHue(j, commandLeft);
          } else {
            setHue(j, commandOn);
          }

          path[j - 1] = path[j - 1] - modifiedPath;
          delay(50);
        }
      }
    }
  } else {
    Serial.println("Error in deletePath function");
  }

}

boolean setHue(uint8_t lightNum, String command) {

  if (clientB.connect(hueBridgeIP, hueBridgePort)) {
    if (clientB.connected()) {
      clientB.print("PUT /api/");
      clientB.print(hueUsername);
      clientB.print("/lights/");
      clientB.print(lightNum);
      clientB.println("/state HTTP/1.1");
      clientB.println("keep-alive");
      clientB.print("Host: ");
      clientB.println(hueBridgeIP);
      clientB.print("Content-Length: ");
      clientB.print(command.length());
      clientB.println("Content-Type: text/plain;charset=UTF-8");
      clientB.println();
      clientB.println(command);
      Serial.print("Sent command for light #");
      Serial.println(lightNum);
    }

    clientB.stop();
    return true;
  } else {
    Serial.println("setHue() - Command failed");
    return false;
  }

}


