#include <SPI.h>
#include <Ethernet.h>

// Ethernet variables
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // MAC address from Ethernet shield sticker under board
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
  if (httpReq.indexOf("GET /command=left?") > -1) {
    left();
  } else if (httpReq.indexOf("GET /command=right?") > -1) {
    right();
  } else if (httpReq.indexOf("GET /command=on?") > -1) {
    on();
  } else if (httpReq.indexOf("GET /command=off?") > -1) {
    off();
  } else {
    Serial.println("DEBUG");
  }
}

void sendHtmlPage(EthernetClient client) {
  client.println("<!DOCTYPE html>");
  client.println("<html>");
  client.println("<head>");
  client.println("<title>Arduino Room Selection</title>");
  client.println("</head>");
  client.println("<body>");
  client.println("<h1>Room Selection</h1>");
  client.println("<p>Click to select the room.</p>");
  client.println("<form method=\"get\" action=\"command=left\">");
  client.println("<input type=\"button\" name=\"left\" value=\"Left\" onclick=\"submit();\">");
  client.println("</form>");
  client.println("<form method=\"get\" action=\"command=right\">");
  client.println("<input type=\"button\" name=\"right\" value=\"Right\" onclick=\"submit();\">");
  client.println("</form>");
  client.println("<form method=\"get\" action=\"command=on\">");
  client.println("<input type=\"button\" name=\"on\" value=\"All On\" onclick=\"submit();\">");
  client.println("</form>");
  client.println("<form method=\"get\" action=\"command=off\">");
  client.println("<input type=\"button\" name=\"off\" value=\"All Off\" onclick=\"submit();\">");
  client.println("</form>");
  client.println("</body>");
  client.println("</html>");
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
