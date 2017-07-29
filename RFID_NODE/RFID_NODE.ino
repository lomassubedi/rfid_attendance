#include <EEPROM.h>

#define SERIAL_READ_TIMEOUT 1000
#define EEPROM_SIZE         255

#define EEPROM_DATA_SIZE    20

#define DEVICE_ID_ADDR      0
#define SSID_ADDR           (EEPROM_DATA_SIZE + DEVICE_ID_ADDR)
#define PASSWORD_ADDR       (EEPROM_DATA_SIZE + SSID_ADDR)

uint8_t serialReadline(char *buff, uint8_t maxbuff, uint16_t timeout) {
  uint16_t buffidx = 0;
  boolean timeoutvalid = true;
  if (timeout == 0) timeoutvalid = false;

  while (true) {
    if (buffidx > maxbuff) {
      //Serial.println(F("SPACE"));
      break;
    }

    while (Serial.available()) {
      char c =  Serial.read();

      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0)   // the first 0x0A is ignored
          continue;

        timeout = 0;         // the second 0x0A is the end of the line
        timeoutvalid = true;
        break;
      }
      buff[buffidx] = c;
      buffidx++;
    }

    if (timeoutvalid && timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  buff[buffidx] = 0;  // null term
  return buffidx;
}

void parseTag(uint8_t *rawBuffr, uint8_t *IDBuffr) {
  
      uint8_t dataBuffr[12];
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

void getEEPROMBytes(uint8_t *bytesBuffr, uint16_t initAddr, uint16_t buffrLen) {
  
  EEPROM.begin(EEPROM_SIZE);  
  
  for(uint16_t i = 0; i < buffrLen; i++)    
    bytesBuffr[i] = EEPROM.read(i + initAddr);  
    
  EEPROM.end();
  return;
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);
  
  uint8_t readEEPBfr[EEPROM_DATA_SIZE];
  
  getEEPROMBytes(readEEPBfr, DEVICE_ID_ADDR, EEPROM_DATA_SIZE);
  Serial.print("\nDevice ID : ");
  String str((const char*)readEEPBfr);
//  Serial.println((const char*)readEEPBfr);
  Serial.println(str);
}

uint8_t readBuffr[16];
uint8_t tagBuffr[6];
uint8_t uartBuffr[250];

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
  } else if(getChar == 'S') {
    
    Serial.println("\n--------- Welcome to RFID attendence user setting ----------");
    Serial.print("Set a Device ID>"); 
       
    char ReadLineBuffr[50];
    
    uint8_t charLen = serialReadline(ReadLineBuffr, 20, SERIAL_READ_TIMEOUT);

    for(uint8_t i = 0; i < charLen; i++)
      Serial.write(ReadLineBuffr[i]);
    
      Serial.print("\nReceived byte length! ");
      Serial.println(charLen);
//    Serial.println((const char *))

    EEPROM.begin(EEPROM_SIZE);
//    for(uint8_t i = 0; i < charLen; i++)
//      EEPROM.write(i, (byte)0);     //clearing
//      delay(1);
//    EEPROM.commit();
////    EEPROM.end();

//    EEPROM.begin(EEPROM_SIZE);
    for(uint8_t i = 0; i < charLen ; i++)
      EEPROM.write(i, (byte)ReadLineBuffr[i]);     
      delay(1); 
    EEPROM.commit();
//    EEPROM.end();    
    

    char eepromReadBack[20];
    uint8_t errorCount = 0;

//    EEPROM.begin(EEPROM_SIZE);
    for(uint8_t i = 0; i < charLen ; i++) {
      eepromReadBack[i] = EEPROM.read(i);
      delay(1);
      if(!(eepromReadBack[i] == ReadLineBuffr[i])) errorCount++; 
    }
    EEPROM.end();
    

    if(errorCount) {
      Serial.println("Failed updating Device ID");
    } else {
      Serial.print("Successfully updated Device ID : ");
      Serial.println((const char*)eepromReadBack);
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


