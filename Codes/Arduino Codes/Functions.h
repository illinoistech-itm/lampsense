
//-----------------------------------------------------------------------------
// Functions.h
//-----------------------------------------------------------------------------

#ifndef _FUNCTIONS_H_INCLUDE_
#define _FUNCTIONS_H_INCLUDE_


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
  } else if (httpReq.indexOf("GET /?command=temp") > -1) {
    showTemp();
  } else if (httpReq.indexOf("GET /?command=arrivedLeft") > -1) {
    deletePath(1, "\n\nPerson arrived at left side.");
  } else if (httpReq.indexOf("GET /?command=arrivedRight") > -1) {
    deletePath(2, "\n\nPerson arrived at right side.");
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
  int analogValue = 0;
  int sender[4];
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

void analyzeMessage(int sender[], int* analogValue) {

  float temp = 0;
  if (sender[3] == SENSOR_TEMP1[3] || sender[3] == SENSOR_TEMP2[3]) { // SENSOR_TEMP1
    temp = convertTemp(analogValue);
    if (temp > 0.0 || temp < 50.0) {
      lastValidTemp = temp;
    }
  } else if (sender[3] == SENSOR_GAS[3]) {
    analyzeGasLevel(analogValue);
  }

}

void analyzeGasLevel(int* analogValue) {

  if (*analogValue > MAX_GAS_LEVEL) {
    gasDetected = true;
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

void setAllLamps(int numPath, String message) {  // numPath: -1 for all off, 0 for all on (normal), 1 for left, 2 for right
  Serial.println(message);

  if (numPath == -1) {
    command = commandOff;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = -1;
      delay(50);
    }

    pathUsed[0] = pathUsed[1] = 0;
  } else if (numPath == 0) {
    command = commandOn;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 0;
      delay(50);
    }

    pathUsed[0] = pathUsed[1] = 0;
  } else if (numPath == 1) {
    command = commandLeft;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 1;
      delay(50);
    }

    pathUsed[0] = pathUsed[1] = 1;
  } else if (numPath == 2) {
    command = commandRight;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 2;
      delay(50);
    }

    pathUsed[0] = pathUsed[1] = 1;
  } else if (numPath == -2) {
    command = message;

    for (int j = 1; j < NUM_LAMPS + 1; j++) {
      setHue(j, command);
      path[j - 1] = 0;
      delay(50);
    }

    pathUsed[0] = pathUsed[1] = 0;
  }
}

void addPath(int addedPath, String message) {  // addedPath: 1 for left, 2 for right
  Serial.println(message);

  // If the lamps are turned off, set their values to 0, so that the sum works accurately
  for (int j = 1; j < NUM_LAMPS + 1; j++) {
    if (path[j - 1] == -1) {
      path[j - 1] = 0;
    }
  }

  if (addedPath == 1) { // left case
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
  } else if (addedPath == 2) { // right case
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

  if (modifiedPath == 1) { // left case
    if (pathUsed[0] == 0) {
      Serial.println("Error: left path is not being used");
    } else {
      pathUsed[0] = 0;

      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathLeft[j - 1] == 1) {
          if (path[j - 1] == 3) {
            setHue(j, commandRight);
          } else {
            setHue(j, commandOn);
          }

          path[j - 1] = path[j - 1] - modifiedPath;
          delay(50);
        }
      }
    }
  } else if (modifiedPath == 2) { // right case
    if (pathUsed[1] == 0) {
      Serial.println("Error: right path is not being used");
    } else {
      pathUsed[1] = 0;

      for (int j = 1; j < NUM_LAMPS + 1; j++) {
        if (pathRight[j - 1] == 1) {
          if (path[j - 1] == 3) {
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

#endif
