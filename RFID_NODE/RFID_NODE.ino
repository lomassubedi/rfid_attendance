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

void clearEEPROM(uint16_t initAddr, uint16_t byteLen) {
  
  EEPROM.begin(EEPROM_SIZE);
    for(uint8_t i = 0; i < byteLen; i++)
      EEPROM.write(i + initAddr, (byte)0);     //clearing
//      delay(1);
   EEPROM.commit();
   EEPROM.end();
   
   return;
}

uint8_t writeEEPROMBytes(uint8_t *bytesBuffr, uint16_t initAddr, uint16_t buffrLen) {
  
   EEPROM.begin(EEPROM_SIZE);
    
    for(uint8_t i = 0; i < buffrLen ; i++)
      EEPROM.write(i + initAddr, (byte)bytesBuffr[i]);     
//      delay(1); 
    EEPROM.commit();
  
    char eepromReadBack[20];
    uint8_t errorCount = 0;

    for(uint8_t i = 0; i < buffrLen ; i++) {
      eepromReadBack[i] = EEPROM.read(i + initAddr);
      delay(1);
      if(!(eepromReadBack[i] == bytesBuffr[i])) errorCount++; 
    }
    EEPROM.end();
    
    return errorCount;
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
  
  delay(2000);
  
  Serial.println("\n-----------------------------\nInitializing hardware modules...\n-----------------------------");
  
  EEPROM.begin(EEPROM_SIZE);

  delay(2000);
  
  uint8_t readEEPBfr[EEPROM_DATA_SIZE];
  
  getEEPROMBytes(readEEPBfr, DEVICE_ID_ADDR, EEPROM_DATA_SIZE);
  Serial.print("Device ID : ");
  String strID((char*)readEEPBfr);
//  Serial.println((const char*)readEEPBfr);
  Serial.println(strID);

  getEEPROMBytes(readEEPBfr, SSID_ADDR, EEPROM_DATA_SIZE);
  Serial.print("WiFi SSID : ");
  String strSSID((char*)readEEPBfr);
//  Serial.println((const char*)readEEPBfr);
  Serial.println(strSSID);

  getEEPROMBytes(readEEPBfr, PASSWORD_ADDR, EEPROM_DATA_SIZE);
  Serial.print("WiFi Password : ");
  String strPassword((char*)readEEPBfr);
//  Serial.println((const char*)readEEPBfr);
  Serial.println(strPassword);
    
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
    
    char readLineBuffr[50];
    uint8_t charLen;

    // --------------- Set Device ID ----------------------
    Serial.print("Set a Device ID>");               
    charLen = serialReadline(readLineBuffr, EEPROM_DATA_SIZE, SERIAL_READ_TIMEOUT);

    for(uint8_t i = 0; i < charLen; i++)
      Serial.write(readLineBuffr[i]);
    Serial.write('\n');

    // Clear EEPROM space for writing device ID
    clearEEPROM(DEVICE_ID_ADDR, EEPROM_DATA_SIZE);
    
    // Write device ID
    if(!writeEEPROMBytes((uint8_t *)readLineBuffr, DEVICE_ID_ADDR, charLen)) {
      Serial.print("Successfully updated device ID : ");
      String str(readLineBuffr);
      Serial.println(str);
    }else {
      Serial.println("Failed updating Device ID !");
    }         

    
    // ------------- Set WiFi SSID -----------------
    Serial.print("Set SSID>");               
    charLen = serialReadline(readLineBuffr, EEPROM_DATA_SIZE, SERIAL_READ_TIMEOUT);

    for(uint8_t i = 0; i < charLen; i++)
      Serial.write(readLineBuffr[i]);
    Serial.write('\n');

    // Clear EEPROM space for writing WiFi SSID
    clearEEPROM(SSID_ADDR, EEPROM_DATA_SIZE);
    
    // Write WiFi SSID
    if(!writeEEPROMBytes((uint8_t *)readLineBuffr, SSID_ADDR, charLen)) {
      Serial.print("Successfully updated wifi SSID : ");
      String str(readLineBuffr);
      Serial.println(str);
    }else {
      Serial.println("Failed updating wifi SSID !");
    }
     
    // ------------- Set WiFi Password -----------------
    Serial.print("Set WiFi Password>");               
    charLen = serialReadline(readLineBuffr, EEPROM_DATA_SIZE, SERIAL_READ_TIMEOUT);

    for(uint8_t i = 0; i < charLen; i++)
      Serial.write(readLineBuffr[i]);
    Serial.write('\n');

    // Clear EEPROM space for writing WiFi password
    clearEEPROM(PASSWORD_ADDR, EEPROM_DATA_SIZE);
    
    // Write WiFi password
    if(!writeEEPROMBytes((uint8_t *)readLineBuffr, PASSWORD_ADDR, charLen)) {
      Serial.print("Successfully updated wifi password : ");
      String str(readLineBuffr);
      Serial.println(str);
    }else {
      Serial.println("Failed updating wifi password !");
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


