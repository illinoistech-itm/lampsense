#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>

// Ethernet variables
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // MAC address from Ethernet shield sticker under board
IPAddress ip(192, 168, 1, 252); // IP address, may need to change depending on network
// IPAddress ip(10, 101, 2, 252); // IP address of the computer lab network
EthernetServer server(80); // create a server at port 80

// Hue variables
const char hueBridgeIP[] = "192.168.1.188"; // IP found for the Philips Hue bridge
// const char hueBridgeIP[] = "10.101.2.54"; // IP address of the computer lab network
const char hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const int hueBridgePort = 80;

const int numLamps = 5;
const int button1 = 7;
const int button2 = 8;
const unsigned long int interval = 1000; // 500 ms

String hueOn;
int hueBri;
long hueHue;
int hueSat;
int path[] = {0, 0, 0, 0, 0}; // -1: off, 0: on for normal lighting 1: left path; 2: right path; 3: left and right paths (sum)
int pathLeft[] = {1, 0, 0, 1, 1};  // 1 for lamp being part of such path; 0 for not
int pathRight[] = {1, 1, 1, 0, 0};
int pathUsed[] = {0, 0};
unsigned int color[numLamps]; // current color for each lamp
String command;
// String commandOn = "{\"on\": true,\"bri\": 215,\"effect\": \"colorloop\",\"alert\": \"select\",\"hue\": 0,\"sat\":0}"; // full command line
String commandOn = "{\"on\": true,\"bri\": 215,\"hue\": 0,\"sat\":0}";
String commandOff = "{\"on\": false,\"bri\": 215,\"hue\": 0,\"sat\":0}";
String commandLeft = "{\"on\": true,\"bri\": 215,\"hue\": 20000,\"sat\":235}";
String commandRight = "{\"on\": true,\"bri\": 215,\"hue\": 50000,\"sat\":235}";
const int debugLED = 13;
int i, cmd;
unsigned long int previousTime = 0xFFFFFFFF, currentTime;

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient clientB;

SoftwareSerial xbee(12, 13); // RX, TX

void setup() {
  pinMode(debugLED, OUTPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  Serial.begin(9600);
  xbee.begin(9600);
  Serial.println("Ethernet.begin initializing");
  Ethernet.begin(mac, ip); // initialize Ethernet device
  server.begin(); // start to listen for clients
  Serial.println("Ethernet.begin executado");
  delay(1000);
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
          sendHtmlPage(client); // send web page
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
  if (digitalRead(button1) == HIGH) {
    deletePath(1, "\n\nPerson arrived at left side");
  } else if (digitalRead(button2) == HIGH) {
    deletePath(2, "\n\nPerson arrived at right side");
  }

  // Evaluate state of lamps
  for (i = 0; i < numLamps; i++) {
    if (path[i] == 3) {
      currentTime = millis();
      if ((previousTime + interval) < currentTime) { // need to fix for millis reseting
        color[i] = 70000 - color[i]; // if color is 20000 turn it to 50000 and vice versa
        Serial.println(color[i]);
        command = "{\"on\": true,\"bri\": 215,\"hue\": " + String(color[i]) + ",\"sat\":235}";
        setHue(i + 1, command);
        previousTime = currentTime;
      }
    }
  }

  // Evaluate if input was given via serial
  if (Serial.available() > 0) {
    cmd = Serial.read();
    if (cmd =='t' || cmd=='T'){
      getTemp();
    }
    if (cmd == 'L' || cmd == 'l') {
      //getPreviousState();
      addPath(1, "Going to left <--");
    }
    else if (cmd == 'R' || cmd == 'r') {
      //getPreviousState();
      addPath(2, "Going to right -->");
    }
    else if (cmd == 'N' || cmd == 'n') {
      //getPreviousState();
      setAllLamps(0, "Turning all on");    // Second parameter to 0 for all lamps on
    }
    else if (cmd == 'F' || cmd == 'f') {
      //getPreviousState();
      setAllLamps(-1, "Turning all off");
    }
  }
}

// Operate the lamps according to the button pressed
void processSelection(String httpReq) {
  if (httpReq.indexOf("GET /?command=left") > -1) {
    addPath(1, "Going to left <--");
  } else if (httpReq.indexOf("GET /?command=right") > -1) {
    addPath(2, "Going to right -->");
  } else if (httpReq.indexOf("GET /?command=on") > -1) {
    setAllLamps(0, "Turning all on"); // Second parameter to 0 for all lamps on
  } else if (httpReq.indexOf("GET /?command=off") > -1) {
    setAllLamps(-1, "Turning all off");
  } else {
    Serial.println("Error in processSelection function");
  }
}

void sendHtmlPage(EthernetClient client) {
  client.println(
    "<!DOCTYPE html>"
    "<html>"
    " <head>"
    "   <title></title>"
    "   <meta charset=\"utf-8\">"
    "   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "   <!-- Latest compiled and minified CSS -->"
    "   <link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\" integrity=\"sha384-1q8mTJOASx8j1Au+a5WDVnPi2lkFfwwEAa8hDDdjZlpLegxhjVME1fgjWPGmkzs7\" crossorigin=\"anonymous\">"
    "   <!-- Optional theme -->"
    "   <link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap-theme.min.css\" integrity=\"sha384-fLW2N01lMqjakBkx3l/M9EahuwpSfeNvV63J5ezn3uZzapT0u7EYsXMjQV+0En5r\" crossorigin=\"anonymous\">"
    "   <script type=\"text/javascript\">"
    "     function getUrlVars() {"
    "       var vars = {};"
    "       var parts = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function(m,key,value) {"
    "         vars[key] = value;"
    "       });"
    "       return vars;"
    "     }"
    "   </script>"
    " </head>"
    " <body>"
    "   <div class=\"container\">"
    "     <script type=\"text/javascript\">"
    "       var command = getUrlVars()['command'];"
    "       console.log(command);"
    "       if (command != undefined) {"
    "         document.write(`<div class=\"alert alert-info fade in\">"
    "           <a href=\"#\" class=\"close\" data-dismiss=\"alert\" aria-label=\"close\">&times;</a>`);"
    "           if (command == 'left') {"
    "             document.write('<strong>Processing...</strong> Lightning the path to the left.');"
    "           } else if (command == 'right') {"
    "             document.write('<strong>Processing...</strong> Lightning the path to the right.');"
    "           } else if (command == 'on') {"
    "             document.write('<strong>Processing...</strong> Turning all the lights on.');"
    "           } else if (command == 'off') {"
    "             document.write('<strong>Processing...</strong> Turning all the lights off.');"
    "           }"
    "         document.write('</div>');"
    "       }"
    "     </script>"
    "     <div class=\"jumbotron\">"
    "       <h1>Arduino + Philips Hue</h1> "
    "       <p>Click on the buttons to operate the lights.</p> "
    "     </div>");
  client.println("     <div class=\"btn-group\">"
    "       <a class=\"btn btn-primary btn-lg\" href=\"?command=left\">"
    "         <span class=\"glyphicon glyphicon-arrow-left\" aria-hidden=\"true\"></span> Left"
    "       </a>"
    "       <a class=\"btn btn-primary btn-lg\" href=\"?command=right\">"
    "         <span class=\"glyphicon glyphicon-arrow-right\" aria-hidden=\"true\"></span> Right"
    "       </a>"
    "       <a class=\"btn btn-primary btn-lg\" href=\"?command=on\">"
    "         <span class=\"glyphicon glyphicon-lamp\" aria-hidden=\"true\"></span> Turn All On"
    "       </a>"
    "       <a class=\"btn btn-primary btn-lg\" href=\"?command=off\">"
    "         <span class=\"glyphicon glyphicon-off\" aria-hidden=\"true\"></span> Turn All Off"
    "       </a>"
    "     </div>"
    "   </div>"
    "   <!-- Latest (compatible) compiled and minified jQuery -->"
    "   <script src=\"https://code.jquery.com/jquery-2.2.4.min.js\" integrity=\"sha256-BbhdlvQf/xTY9gja0Dq3HiwQF8LaCRTXxZKRutelT44=\" crossorigin=\"anonymous\"></script>"
    "   <!-- Latest compiled and minified JavaScript -->"
    "   <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\" integrity=\"sha384-0mSbJDEHialfmuBBQP6A4Qrprq5OVfW37PRR3j5ELqxss1yVqOtnepnHVP9aJ7xS\" crossorigin=\"anonymous\"></script>"
    " </body>"
    "</html>"
  );
}

void getTemp(){
  byte discard, analogHigh, analogLow;
  int analogValue = 0;
  float temp = 0;
  boolean received = false;
  
  // Serial.println("Trying to read temperature");
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
    delay(5000);
    setAllLamps(0, commandOn);
    pathUsed[0] = pathUsed[1] = 0;
    received = false;
  }
}

void tempToLamp(float* temp) {
  if (*temp < 15.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 55000,\"sat\":235}";
    setAllLamps(-2, command);
  }
  else if (*temp < 25.0) {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 30000,\"sat\":235}";
    setAllLamps(-2, command);
  }
  else {
    command = "{\"on\": true,\"bri\": 215,\"hue\": 5000,\"sat\":235}";
    setAllLamps(-2, command);
  }
}

void setAllLamps(int numPath, String message) {  // numPath: -1 for all off, 0 for all on (normal), 1 for left, 2 for right
  Serial.println(message);
  if (numPath == -1) {
    command = commandOff;
    for (int j = 1; j < numLamps + 1; j++) {
      setHue(j, command);
      path[j - 1] = -1;
      delay(50);
    }
    pathUsed[0]=pathUsed[1]=0;
  }
  else if (numPath == 0) {
    command = commandOn;
    for (int j = 1; j < numLamps + 1; j++) {
      setHue(j, command);
      path[j - 1] = 0;
      delay(50);
    }
    pathUsed[0]=pathUsed[1]=0;
  }
  else if (numPath == 1) {
    command = commandLeft;
    for (int j = 1; j < numLamps + 1; j++) {
      setHue(j, command);
      path[j - 1] = 1;
      delay(50);
    }
    pathUsed[0]=pathUsed[1]=1;
  }
  else if (numPath == 2) {
    command = commandRight;
    for (int j = 1; j < numLamps + 1; j++) {
      setHue(j, command);
      path[j - 1] = 2;
      delay(50);
    }
    pathUsed[0]=pathUsed[1]=1;
  }
  else if (numPath == -2) {
    command = message;
    for (int j = 1; j < numLamps + 1; j++) {
      setHue(j, command);
      path[j - 1] = 0;
      delay(50);
    }
    pathUsed[0]=pathUsed[1]=0;
  }
}

void addPath(int addedPath, String message) {  // addedPath: 1 for left, 2 for right
  Serial.println(message);
  if (addedPath == 1) { // left case
    if (pathUsed[0] == 1) Serial.println("Error: left path is already being used");
    else {
      pathUsed[0] = 1;    // change - left path being used
      for (int j = 1; j < numLamps + 1; j++) {
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
      for (int j = 1; j < numLamps + 1; j++) {
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
      for (int j = 1; j < numLamps + 1; j++) {
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
      for (int j = 1; j < numLamps + 1; j++) {
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
  }
  else {
    Serial.println("setHue() - Command failed");
    return false;
  }
}

// boolean getHue(int lightNum) {
//   if (clientB.connect(hueBridgeIP, hueBridgePort))
//   {
//     clientB.print("GET /api/");
//     clientB.print(hueUsername);
//     clientB.print("/lights/");
//     clientB.print(lightNum);
//     clientB.println(" HTTP/1.1");
//     clientB.print("Host: ");
//     clientB.println(hueBridgeIP);
//     clientB.println("Content-type: application/json");
//     clientB.println("keep-alive");
//     clientB.println();
//     while (clientB.connected())
//     {
//       if (clientB.available())
//       {
//         //read the current bri, hue, sat, and whether the light is on or off
//         clientB.findUntil("\"on\":", '\0');
//         hueOn = (clientB.readStringUntil(','));

//         clientB.findUntil("\"bri\":", '\0');
//         hueBri = clientB.readStringUntil(',').toInt();

//         clientB.findUntil("\"hue\":", '\0');
//         hueHue = clientB.readStringUntil(',').toInt();

//         clientB.findUntil("\"sat\":", '\0');
//         hueSat = clientB.readStringUntil(',').toInt();
//         break;
//       }
//     }
//     clientB.stop();
//     return true;
//   }
//   else
//     Serial.println("getHue() - Command failed");
//   return false;
// }

// void getPreviousState() {
//   Serial.println("\n\nReading current state");
//   getHue(1);
//   delay(10);
//   getMessagesDebug("Light 1 - on: ");
//   getHue(2);
//   delay(10);
//   getMessagesDebug("Light 2 - on: ");
//   getHue(3);
//   delay(10);
//   getMessagesDebug("Light 3 - on: ");
// }

// void getMessagesDebug(String message) {
//   Serial.print(message);
//   Serial.print(hueOn);
//   Serial.print(", hueBri: ");
//   Serial.print(hueBri);
//   Serial.print(", hueHue: ");
//   Serial.print(hueHue);
//   Serial.print(", hueSat: ");
//   Serial.print(hueSat);
//   Serial.println();
// }
