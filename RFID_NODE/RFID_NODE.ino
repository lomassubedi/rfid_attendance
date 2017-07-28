void setup() {
  Serial.begin(9600);
}

uint8_t readBuffr[20];

void loop() {

  uint8_t getChar = Serial.read();
  uint8_t i = 0;
  bool flagPacketReadComplete = false;
  if(getChar == 0x02) {       
    Serial.flush();
    readBuffr[i++] = getChar;    
    while(Serial.available() > 0) {    
      getChar = Serial.read();
      readBuffr[i++] = getChar;
      Serial.flush();
      if(getChar == 0x03) {
        flagPacketReadComplete = true;
        break;
      }
    }
  }
  
  if(flagPacketReadComplete) {
    Serial.println((const char *)readBuffr);
    memset(readBuffr, 0, 20);
  }
}


