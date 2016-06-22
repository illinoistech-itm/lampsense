//////////////////////////////////////////////////////////////////
//©2011 bildr
//Released under the MIT License - Please reuse change and share
//Monitors the serial port to see if there is data available.
//'a' turns on the LED on pin 13. 'b' turns it off.
//////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);      //initialize serial
  pinMode(13, OUTPUT);    //set pin 13 as output
}

void loop() {
  while(Serial.available()){  //is there anything to read?
  char getData = Serial.read();  //if yes, read it

  if(getData == 'a'){    
      digitalWrite(13, HIGH);
  }else if(getData == 'b'){
      digitalWrite(13, LOW);
  }
  }
}
