/*
  ==============================================================================

    Project: Intelligent Lights and Sensor Node Using XBees

    Description:

    Circuit:

    Created by:
    Andreive Giovanini Silva, Breno Soares Martins, Pedro Henrique Gualdi

    URL: https://github.com/Illinois-tech-ITM/BSMP-2016-Intelligent-Lights

  ==============================================================================
*/

/* Libraries */
#include <SPI.h>
#include <Ethernet.h>
// #include <SoftwareSerial.h>

/* Ethernet variables */
// Arduino Parameters
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // MAC address from Ethernet shield sticker under board
IPAddress ip(192, 168, 1, 252); // IP address, may need to change depending on network
EthernetServer server(80); // create a server at port 80
// Database connection variables
IPAddress serverDB(192, 168, 1, 153);
EthernetClient clientDB;

/* Constant variables */
// Philips Hue variables
const char    hueBridgeIP[] = "192.168.1.111"; // IP found for the Philips Hue bridge
const char    hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const uint8_t hueBridgePort = 80;

// Constant identifiers
const uint8_t BUTTON1    = 7;
const uint8_t BUTTON2    = 8;
const uint8_t DEBUG_LED  = 13;
const uint8_t NUM_LAMPS  = 6;
const uint8_t LAMP_OFF   = 0;
const uint8_t LAMP_ON    = 1;
const uint8_t LAMP_LEFT  = 2;
const uint8_t LAMP_RIGHT = 4;
const uint8_t LAMP_BOTH  = 6;
const byte SENSOR_TEMP1[] = {0x40, 0xC6, 0x74, 0xF3};
const byte SENSOR_TEMP2[] = {0x40, 0xC6, 0x73, 0xFB};
const byte SENSOR_GAS[]   = {0x40, 0xC6, 0x73, 0xE7};
// Constant parameters
const uint64_t INTERVAL = 1000, INTERVAL_DB = 5000;
const uint16_t MAX_GAS_LEVEL = 100;
// Constant strings
// String commandOn = "{\"on\": true,\"bri\": 215,\"effect\": \"colorloop\",\"alert\": \"select\",\"hue\": 0,\"sat\":0}"; // full command line
const char commandOn[]    = "{\"on\": true,\"bri\": 40,\"hue\": 0,\"sat\":0}";
const char commandOff[]   = "{\"on\": false,\"bri\": 40,\"hue\": 0,\"sat\":0}";
const char commandLeft[]  = "{\"on\": true,\"bri\": 40,\"hue\": 20000,\"sat\":235}";
const char commandRight[] = "{\"on\": true,\"bri\": 40,\"hue\": 50000,\"sat\":235}";
// Constants for paths: 1 for lamp being part of such path, 0 for not
const uint8_t pathLeft[NUM_LAMPS]  = {1, 0, 0, 1, 1, 1};
const uint8_t pathRight[NUM_LAMPS] = {1, 1, 1, 0, 0, 0};

/* Global variables */
int8_t   path[NUM_LAMPS] = {0, 0, 0, 0, 0, 0}; // 0: off, 1: on for normal lighting, 2: left path; 4: right path; 6: left and right paths (sum)
boolean  pathUsed[]      = {false, false};             // Length equal to number of different paths
uint8_t  i, cmd;
unsigned int color[NUM_LAMPS];                     // current color for each lamp
unsigned int colorGas = 0;
unsigned long int previousTimePath = 0, previousTimeGas = 0, previousTimeDB = 0, currentTime;
float    lastValidTemp;
String  command;
boolean gasDetected = false;

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient clientB;

// SoftwareSerial xbee(12, 13); // RX, TX

void setup() {

  // Initialize pins
  pinMode(DEBUG_LED, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  // Initialize Serial services
  Serial.begin(9600);
  Serial1.begin(9600);

  // Initialize Ethernet server
  Serial.println("Ethernet.begin initializing");
  Ethernet.begin(mac, ip); // initialize Ethernet device
  server.begin(); // start to listen for clients
  Serial.println("Ethernet.begin executado");
  delay(100);
  Serial.println("Ready for commands");
}

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
    deletePath(1, "\n\nPerson arrived at left side.");
  } else if (digitalRead(BUTTON2) == HIGH) {
    deletePath(2, "\n\nPerson arrived at right side.");
  }

  // Evaluate state of lamps
  if (gasDetected) {
    // Serial.println("gasDetected");
    gasToLamps();
  }
  else {
    for (i = 0; i < NUM_LAMPS; i++) {
      if (path[i] >= LAMP_BOTH) {
        currentTime = millis();
        if ((previousTimePath + INTERVAL) < currentTime) { // need to fix for millis reseting
          color[i] = 70000 - color[i]; // if color is 20000 turn it to 50000 and vice versa
          Serial.println(color[i]);
          command = "{\"on\": true,\"bri\": 40,\"hue\": " + String(color[i]) + ",\"sat\":235}";
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
    } else if (cmd == 'L' || cmd == 'l') {
      //getPreviousState();
      addPath(LAMP_LEFT, "Going to left <--");
    } else if (cmd == 'R' || cmd == 'r') {
      //getPreviousState();f
      addPath(LAMP_RIGHT, "Going to right -->");
    } else if (cmd == 'N' || cmd == 'n') {
      //getPreviousState();
      setAllLamps(LAMP_ON, "Turning all on"); // Second parameter to 0 for all lamps on
    } else if (cmd == 'F' || cmd == 'f') {
      //getPreviousState();
      setAllLamps(LAMP_OFF, "Turning all off");
    } else if (cmd == 'G' || cmd == 'g') {
      //getPreviousState();
      gasDetected = false;
      setAllLamps(LAMP_ON, "Turning all on");
    }
  }

  // Evaluate if input was given via Xbee serial port
  getSensorData();

  // Send the temperature via GET every X (INTERVAL_DB) milliseconds
  /*currentTime = millis();
    if ((previousTimeDB + INTERVAL_DB) < currentTime) {
    sendTempViaGet();
    previousTimeDB = currentTime;
    }*/
}

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
    "document.write('<strong>Processing...</strong> Lighting up the path to the left.');"
    "} else if (command == 'right') {"
    "document.write('<strong>Processing...</strong> Lighting up the path to the right.');"
    "} else if (command == 'on') {"
    "document.write('<strong>Processing...</strong> Turning all the lights on.');"
    "} else if (command == 'off') {"
    "document.write('<strong>Processing...</strong> Turning all the lights off.');"
    "} else if (command == 'temp') {"
    "document.write('<strong>Processing...</strong> Coloring the lights according to the temperature.');"
    "} else if (command == 'arrivedLeft') {"
    "document.write('<strong>Processing...</strong> Resetting the lights in the path to the left.');"
    "} else if (command == 'arrivedRight') {"
    "document.write('<strong>Processing...</strong> Resetting the lights in the path to the right.');"
    "}"
    "document.write('</div>');"
    "}"
    "</script>"
  );

  client.println(
    "<div class=\"jumbotron text-center\">"
    "<h1>LampSense</h1>"
    "</div>"
    "<div class=\"text-center\">"
    "<div class=\"btn-group-vertical\">"
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
    "<span class=\"glyphicon glyphicon-triangle-left\" aria-hidden=\"true\"></span> Arrived Left"
    "</a>"
    "<a class=\"btn btn-primary btn-lg\" href=\"?command=arrivedRight\">"
    "<span class=\"glyphicon glyphicon-triangle-right\" aria-hidden=\"true\"></span> Arrived Right"
    "</a>"
    "</div>"
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

void getSensorData() {

  byte discard, analogHigh, analogLow;
  int analogValue = 0, i;
  byte sender[4];
  float temp = 0;

  // make sure everything we need is in the buffer
  if (Serial1.available() >= 21) {
    // look for the start byte
    if (Serial1.read() == 0x7E) {
      // read the variables that we're not using out of the buffer
      // read the variables that we're not using out of the buffer
      for (i = 1; i < 8 ; i++) discard = Serial1.read();
      // address of sender (bytes from 8 to 11)
      for (i = 8; i < 12; i++) sender[i - 8] = Serial1.read();
      for (i = 12; i < 19; i++) discard = Serial1.read();
      analogHigh = Serial1.read();
      analogLow = Serial1.read();
      analogValue = analogLow + (analogHigh * 256);
      analyzeMessage(sender, &analogValue);
    }
  }

}

void analyzeMessage(byte sender[], int* analogValue) {

  float temp = 0;
  if (sender[3] == SENSOR_TEMP1[3] || sender[3] == SENSOR_TEMP2[3]) { // In this case, only last byte of sender is sufficient to distinguish
    temp = convertTemp(analogValue);
    if (temp > 0.0 && temp < 50.0) {        // discard "wrong" values, i.e. values uncommon. Lower limit may be adjusted
      lastValidTemp = temp;
    }
  } else if (sender[3] == SENSOR_GAS[3]) {
    analyzeGasLevel(analogValue);
  }

}

void analyzeGasLevel(int* analogValue) {

  Serial.print("gas value: "); Serial.println(*analogValue);
  if (*analogValue > MAX_GAS_LEVEL) {
    gasDetected = true;
  } else if ((*analogValue < MAX_GAS_LEVEL) && gasDetected) {
    setAllLamps(LAMP_ON, "No more gas");
    gasDetected = false;
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
  setAllLamps(LAMP_ON, "Turning all lamps on");
  pathUsed[0] = pathUsed[1] = 0;
}

// Color the lamps according to the temperature
void tempToLamp(float* temp) {
  if (*temp < 15.0) {
    command = "{\"on\": true,\"bri\": 40,\"hue\": 55000,\"sat\":235}";
    setAllLamps(-2, command);
  } else if (*temp < 20.0) {
    command = "{\"on\": true,\"bri\": 40,\"hue\": 42000,\"sat\":235}";
    setAllLamps(-2, command);
  } else if (*temp < 25.0) {
    command = "{\"on\": true,\"bri\": 40,\"hue\": 35000,\"sat\":255}";
    setAllLamps(-2, command);
  } else if (*temp < 30.0) {
    command = "{\"on\": true,\"bri\": 40,\"hue\": 10000,\"sat\":235}";
    setAllLamps(-2, command);
  } else {
    command = "{\"on\": true,\"bri\": 40,\"hue\": 5000,\"sat\":235}";
    setAllLamps(-2, command);
  }
}

void gasToLamps() {

  currentTime = millis();
  if ((previousTimeGas + INTERVAL) < currentTime) { // need to fix for millis reseting
    colorGas = 10000 - colorGas; // if color is 5000 turn it to 0 and vice versa
    Serial.println(colorGas);
    command = "{\"on\": true,\"bri\": 40,\"hue\": " + String(colorGas) + ",\"sat\":235}";
    setAllLamps(-2, command);
    previousTimeGas = currentTime;
  }

}

void setAllLamps(int numPath, String message) {  // numPath: -1 for all off, 0 for all on (normal), 1 for left, 2 for right
  Serial.println(message);

  if (numPath == LAMP_OFF) {
    command = commandOff;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = LAMP_OFF;
      delay(50);
    }

    pathUsed[0] = pathUsed[1] = 0;
  } else if (numPath == LAMP_ON) {
    command = commandOn;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = LAMP_ON;
      delay(50);
    }

    pathUsed[0] = pathUsed[1] = 0;
  } else if (numPath == LAMP_LEFT) {
    command = commandLeft;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = LAMP_LEFT;
      delay(50);
    }

    pathUsed[0] = 1;
    pathUsed[1] = 0;
  } else if (numPath == LAMP_RIGHT) {
    command = commandRight;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = LAMP_RIGHT;
      delay(50);
    }

    pathUsed[0] = 0;
    pathUsed[1] = 1;
  } else {
    command = message;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = LAMP_OFF;
      delay(50);
    }

    pathUsed[0] = pathUsed[1] = 0;
  }
}

void addPath(int addedPath, String message) {  // addedPath: 1 for left, 2 for right
  Serial.println(message);

  /*// If the lamps are turned off, set their values to 0, so that the sum works accurately
    for (int j = 1; j < NUM_LAMPS + 1; j++) {
    if (path[j - 1] == -1) {
      path[j - 1] = 0;
    }
    }*/

  if (addedPath == LAMP_LEFT) { // left case
    if (pathUsed[0] == 1) {
      Serial.println("Error: left path is already being used");
    } else {
      pathUsed[0] = 1; // change - left path being used

      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathLeft[j - 1] == 1) {
          setHue(j, commandLeft);
          path[j - 1] = path[j - 1] + addedPath;
          color[j - 1] = 20000;
          delay(50);
        }
      }
    }
  } else if (addedPath == LAMP_RIGHT) { // right case
    if (pathUsed[1] == 1) {
      Serial.println("Error: right path is already being used");
    } else {
      pathUsed[1] = 1; // change - right path being used

      for (int j = 1; j < NUM_LAMPS + 1; j++) {
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
  Serial.println(message);

  if (modifiedPath == LAMP_LEFT) { // left case
    if (pathUsed[0] == 0) {
      Serial.println("Error: left path is not being used");
    } else {
      pathUsed[0] = 0;

      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathLeft[j - 1] == 1) {
          if (path[j - 1] >= 6) {
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
    if (pathUsed[1] == 0) {
      Serial.println("Error: right path is not being used");
    } else {
      pathUsed[1] = 0;

      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathRight[j - 1] == 1) {
          if (path[j - 1] >= 6) {
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

boolean setHue(int lightNum, String command) {
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
