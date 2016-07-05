#include <SPI.h>
#include <Ethernet.h>

// Ethernet variables
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // MAC address from Ethernet shield sticker under board
IPAddress ip(192, 168, 1, 252); // IP address, may need to change depending on network
EthernetServer server(80); // create a server at port 80

// HUE variables
const char hueBridgeIP[] = "192.168.1.188"; // IP found for the Philips Hue bridge
const char hueUsername[] = "newdeveloper";  // Developer name created when registering a user
const int hueBridgePort = 80;
String command;
String commandOn = "{\"on\": true,\"bri\": 215,\"hue\": 0,\"sat\":0}";
String commandOff = "{\"on\": false}";

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetClient clientB;

void setup() {
  Ethernet.begin(mac, ip); // initialize Ethernet device
  server.begin(); // start to listen for clients
  Serial.begin(9600); // for diagnostics
  pinMode(2, OUTPUT); // LED on pin 2
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
}

// switch LED and send back HTML for LED checkbox
void processSelection(String httpReq) {
  if (httpReq.indexOf("GET /?command=left") > -1) {
    left();
  } else if (httpReq.indexOf("GET /?command=right") > -1) {
    right();
  } else if (httpReq.indexOf("GET /?command=on") > -1) {
    on();
  } else if (httpReq.indexOf("GET /?command=off") > -1) {
    off();
  } else {
    Serial.println("DEBUG");
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

void left() {
  command = "{\"on\": true,\"bri\": 215,\"hue\": 25178,\"sat\":235}";
  Serial.println("Going to left <--");
  setHue(1, command);
  delay(100);
  setHue(2, command);
  delay(100);
  setHue(3, commandOff);
  delay(500);
}

void right() {
  command = "{\"on\": true,\"bri\": 215,\"hue\": 50000,\"sat\":235}";
  Serial.println("Going to right -->");
  setHue(1, command);
  delay(100);
  setHue(3, command);
  delay(100);
  setHue(2, commandOff);
  delay(500);
}

void on() {
  Serial.println("Turning all on");
  setHue(1, commandOn);
  delay(100);
  setHue(2, commandOn);
  delay(100);
  setHue(3, commandOn);
  delay(500);
}

void off() {
  Serial.println("Turning all off");
  setHue(1, commandOff);
  delay(100);
  setHue(2, commandOff);
  delay(100);
  setHue(3, commandOff);
  delay(500);
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
