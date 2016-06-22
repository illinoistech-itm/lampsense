import processing.serial.*;

Serial myPort;   // The serial port

float tempC;
float tempF;
int yDist;
PFont font12;
PFont font24;
float[] tempHistory = new float[100];

int addr1;
int addr2;
int addr3;
int addr4;

void setup() {

  //setup fonts for use throughout the application
  font12 = loadFont("Verdana-12.vlw");
  font24 = loadFont("Verdana-24.vlw");

  //set the size of the window
  size(550, 250);


  //fill tempHistory with default temps
  for (int index = 0; index<100; index++)
    tempHistory[index] = 0;

  // I know that the first port in the serial list on my mac
  // is always my  FTDI adaptor, so I open Serial.list()[0].
  // On Windows machines, this generally opens COM1.
  // Open whatever port is the one you're using.
  println(Serial.list());
  myPort = new Serial(this, Serial.list()[0], 9600);
}

void draw() {

  if (myPort.available() >= 21) { // Wait for coordinator to recieve full XBee frame
    if (myPort.read() == 0x7E) { // Look for 7E because it is the start byte
      for (int i = 1; i<19; i++) { // Skip through the frame to get to the unique 32 bit address
        //get each byte of the XBee address
        if (i == 8) { 
          addr1 = myPort.read();
        } else if (i==9) { 
          addr2 = myPort.read();
        } else if (i==10) { 
          addr3 = myPort.read();
        } else if (i==11) { 
          addr4 = myPort.read();
        } else { 
          int discardByte = myPort.read();
        } //else throwout byte we don't need it
      }
      int analogMSB = myPort.read(); // Read the first analog byte data
      int analogLSB = myPort.read(); // Read the second byte
      float volt = calculateXBeeVolt(analogMSB, analogLSB);//Convert analog values to voltage values
      println(indentifySensor(addr1, addr2, addr3, addr4)); //get identity of XBee and print it
      print("Temperature in F: ");
      println(String.format("%.1f", calculateTempF(volt))); //calculate temperature value from voltage value

      //refresh the background to clear old data
      background(123);

      //draw the temp rectangle
      colorMode(RGB, 160);  //use color mode sized for fading
      stroke (0);
      rect (49, 19, 22, 162);
      //fade red and blue within the rectangle
      for (int colorIndex = 0; colorIndex <= 160; colorIndex++)
      {
        stroke(160 - colorIndex, 0, colorIndex);
        line(50, colorIndex + 20, 70, colorIndex + 20);
      }

      //write reference values
      fill(0, 0, 0);
      textFont(font12);
      textAlign(RIGHT);
      text("257 F", 45, 25);
      text("-40 F", 45, 187);

      //draw triangle pointer
      yDist = int(calculateTempF(volt));
      stroke(0);
      triangle(75, yDist + 20, 85, yDist + 15, 85, yDist + 25);

      //write the temp in C and F
      fill(0, 0, 0);
      textFont(font24);
      textAlign(LEFT);
      //text(str(int(tempC)) + " C", 115, 37);
      //tempF = ((tempC*9)/5) + 32;
      //text(calculateTempF(volt));
      text(indentifySensor(addr1, addr2, addr3, addr4), 115, 95);
      text("Temperature in F: " + String.format("%.1f", calculateTempF(volt)) + " F", 115, 135);
    }
  }
}
//Function takes in the XBee address and returns the identity of the Xbee that sent the temperature data
String indentifySensor(int a1, int a2, int a3, int a4) {
  int rout1[] = {64, 159, 115, 24}; //Arrays are the 32 bit address of the two XBees routers
  int end1[] = {64, 139, 174, 61};
  int end2[] = {64, 166, 42, 119};
  if (a1==rout1[0] && a2==rout1[1] && a3==rout1[2] && a4==rout1[3]) { //Check if Sensor 1
    return "Temperature from router - sensor 1";
  } //temp data is from XBee one
  else if (a1==end1[0] && a2==end1[1] && a3==end1[2] && a4==end1[3]) {//Check if Sensor 2
    return "Temperature from end - sensor 2";
  } //temp data is from XBee two
  else if (a1==end2[0] && a2==end2[1] && a3==end2[2] && a4==end2[3]) {//Check if Sensor 2
    return "Temperature from end - sensor 3";
  } //temp data is from XBee two
  else { 
    return "I don't know this sensor";
  }  //Data is from an unknown XBee
}

//this function calculates temp in F from temp sensor
float calculateTempF(float v1) {
  float temp = 0;
  //calculate temp in C, .75 volts is 25 C. 10mV per degree
  if (v1 < .75) { 
    temp = 25 - ((.75-v1)/.01);
  } //if below 25 C
  else if (v1 == .75) {
    temp = 25;
  } else { 
    temp = 25 + ((v1 -.75)/.01);
  } //if above 25
  //convert to F
  temp =((temp*9)/5) + 32;
  return temp;
}

//This function takes an XBee analog pin reading and converts it to a voltage value
float calculateXBeeVolt(int analogMSB, int analogLSB) {
  int analogReading = analogLSB + (analogMSB * 256); //Turn the two bytes into an integer value
  float volt = ((float)analogReading / 1023)*1.23; //Convert the analog value to a voltage value
  return volt;
}