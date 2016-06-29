#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // MAC address from Ethernet shield sticker under board
IPAddress ip(192, 168, 1, 252); // IP address, may need to change depending on network
EthernetServer server(80); // create a server at port 80

String httpReq; // stores the HTTP request

void setup() {
  Ethernet.begin(mac, ip); // initialize Ethernet device
  server.begin(); // start to listen for clients
  Serial.begin(9600); // for diagnostics
  pinMode(2, OUTPUT); // LED on pin 2
}

void loop() {
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
          // Serial.print(httpReq);
          processSelection();
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
void processSelection() {
  if (httpReq.indexOf("GET /room=1?") > -1) {
    Serial.println("Room 1 selected...");
  }

  if (httpReq.indexOf("GET /room=2?") > -1) {
    Serial.println("Room 2 selected...");
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
  client.println("<form method=\"get\" action=\"room=1\">");
  client.println("<input type=\"button\" name=\"room\" value=\"Room 1\" onclick=\"submit();\">");
  client.println("</form>");
  client.println("<form method=\"get\" action=\"room=2\">");
  client.println("<input type=\"button\" name=\"room\" value=\"Room 2\" onclick=\"submit();\">");
  client.println("</form>");
  client.println("</body>");
  client.println("</html>");
}
