void parseTag(uint8_t *rawBuffr, uint8_t *IDBuffr) {
  
      uint8_t dataBuffr[12];
//      uint8_t dataBuffr[] = "2E00AF0EC9B4";
      uint8_t indx, j = 0;
      uint8_t tmpMSB, tmpLSB;
    
      memcpy(dataBuffr, &rawBuffr[1], 12);
      
    for(indx = 0; indx < 12; indx += 2) {           
      
      tmpMSB = dataBuffr[indx];
      tmpLSB = dataBuffr[indx + 1];
      
      if(tmpMSB > 0x39) tmpMSB = ((10 + tmpMSB) - 0x41);
      else tmpMSB = tmpMSB - 0x30;
      
      if(tmpLSB > 0x39) tmpLSB = ((10 + tmpLSB) - 0x41);
      else tmpLSB = tmpLSB - 0x30;     
      
      IDBuffr[j++] = (tmpMSB << 4 | tmpLSB); //dataBuffr[indx]            
    }      
//    Serial.print("\nData ----------  ");
//    Serial.println(IDBuffr[0]);
//    Serial.println(IDBuffr[1]);
//    Serial.println(IDBuffr[2]);
//    Serial.println(IDBuffr[3]);
//    Serial.println(IDBuffr[4]);
//    Serial.println(IDBuffr[5]);
////    Serial.println("  ---------- ");
  return;
}

bool validateCheckSum(uint8_t *IDBuffr) {
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 5; i++) {
      sum += IDBuffr[i];
    }
    
    sum = sum & 0xFF;
                  
    if(sum == IDBuffr[5]) return true;
    else return false;
}

uint32_t getTagValue(uint8_t *IDBuffr) {
  return (IDBuffr[1] << 24 | IDBuffr[2] << 16 | IDBuffr[3] << 8 | IDBuffr[4]);
}

void setup() {
  Serial.begin(9600);
}

uint8_t readBuffr[20];
//uint8_t dataBuffr[12]
uint8_t tagBuffr[6];

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
      } else {
        flagPacketReadComplete = false;
      }
    }
  }

  uint32_t readTag;
  
  if(flagPacketReadComplete) {           
    parseTag(readBuffr, tagBuffr);            
    if(validateCheckSum(tagBuffr)) {
      readTag = getTagValue(tagBuffr);      
      Serial.println(readTag);
    } else {
       Serial.println("Check sum error !");      
    }    
    memset(readBuffr, 0, 20); 
  }
  
}


