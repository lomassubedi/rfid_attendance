#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#include "customCodes.h"

#define DEBUG_SERIAL                // Turn ON serial debug 

#define SERIAL_READ_TIMEOUT         2000
#define EEPROM_SIZE                 3072

#define EEPROM_DATA_SIZE            20

#define DEVICE_ID_ADDR              0
#define SSID_ADDR                   (EEPROM_DATA_SIZE + DEVICE_ID_ADDR)
#define PASSWORD_ADDR               (EEPROM_DATA_SIZE + SSID_ADDR)

#define FAILED_TAG_STACK_PTR_ADDR   (EEPROM_DATA_SIZE + PASSWORD_ADDR)
#define FAILED_TAG_STACK_ADDR       (FAILED_TAG_STACK_PTR_ADDR + FAILED_TAG_STACK_PTR_ADDR)  

#define POST_RESPONSE_PAYLOAD_SIZE  100
#define POST_PAYLOAD_SIZE           200

volatile uint16_t failedTagStackPtr = 0;

const char url[] = "http://192.168.100.6:4044/data";

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

uint16_t intReadEEPROM(uint16_t p_address) {
  
     uint8_t bytesInt[2];    
     
     getEEPROMBytes(bytesInt, p_address, sizeof(bytesInt));
                         
     return ((bytesInt[0] << 0) & 0xFF) + ((bytesInt[1] << 8) & 0xFF00);
}
     
uint8_t intWriteEEPROM(uint16_t p_address, uint16_t p_value) {
    
    uint8_t bytesInt[2];
    
     bytesInt[0] = ((p_value >> 0) & 0xFF);
     bytesInt[1] = ((p_value >> 8) & 0xFF);
          
     if(writeEEPROMBytes(bytesInt, p_address, sizeof(bytesInt))) {
      return ERROR_CODE;
     } else {
      return SUCESS_CODE;
     }         
}

uint8_t addIDToEEPROMStack(uint32_t ID) {
  
  uint8_t addrCtr = 0;
  uint8_t IDbyte[4];
  
  IDbyte[0] = ((ID >> 0) & 0xFF);
  IDbyte[1] = ((ID >> 8) & 0xFF);
  IDbyte[2] = ((ID >> 16) & 0xFF);
  IDbyte[3] = ((ID >> 24) & 0xFF);
    
  uint16_t stackPtr = intReadEEPROM(FAILED_TAG_STACK_PTR_ADDR);
  uint16_t byteAddr = (stackPtr * sizeof(uint32_t)) + 1;
  byteAddr += FAILED_TAG_STACK_ADDR;
  
//  uint8_t errorEEPROM = writeEEPROMBytes(IDbyte, byteAddr, sizeof(uint32_t));

//  Serial.print("stackPointer : ");
//  Serial.print(stackPtr);
//  Serial.print(" EEPROM ERROR  : ");
//  Serial.println(errorEEPROM);
  
  
  if(writeEEPROMBytes(IDbyte, byteAddr, sizeof(uint32_t))) {
    return ERROR_CODE;
  } else {
    stackPtr++;
    intWriteEEPROM(FAILED_TAG_STACK_PTR_ADDR, stackPtr);
    return SUCESS_CODE;        
  }      
}

uint32_t getIDFromEEPROMStack() {
  
  uint8_t addrCtr = 0;
  uint8_t IDbyte[4];   
    
  uint16_t stackPtr = intReadEEPROM(FAILED_TAG_STACK_PTR_ADDR);  
  
  uint16_t byteAddr = (stackPtr * sizeof(uint32_t)) - sizeof(uint32_t);
  byteAddr += FAILED_TAG_STACK_ADDR;
  
  getEEPROMBytes(IDbyte, byteAddr, sizeof(uint32_t));    
  stackPtr--;
  intWriteEEPROM(FAILED_TAG_STACK_PTR_ADDR, stackPtr);
  return (uint32_t)(IDbyte[3] << 24 | IDbyte[2] << 16 | IDbyte[1] << 8 | IDbyte[0]);
}

void encodeJson(char * jsonArry, uint16_t arrySize, uint32_t tagValue) {
  
  const char dataString[] = "data=";
  const uint8_t sizeCharsDataString = sizeof(dataString) - 1; // Omit null character
  
  sprintf(jsonArry, dataString);
   
  StaticJsonBuffer<200> jsonBuffer;  
  JsonObject& root = jsonBuffer.createObject();  
  
    root["tag"] = tagValue;    
//    root["time"] = "2017-01-01T01:02:03";   
  
   root.printTo(&jsonArry[sizeCharsDataString], (arrySize - sizeCharsDataString));
    
  #ifdef DEBUG_SERIAL    
    Serial.println(jsonArry);
  #endif
  return;
}

uint8_t postToServer(const char *  url, const char * jsonCharArray) {

  String payload;
  char payLoad[POST_RESPONSE_PAYLOAD_SIZE];
  
  if(WiFi.status() == WL_CONNECTED){   // Check WiFi connection status
      
       HTTPClient http;    //Declare object of class HTTPClient
  
       http.begin(url);  //Specify request destination
       
       http.addHeader("Content-Type", "text/plain");  //Specify content-type header
       http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
       int httpCode = http.POST(jsonCharArray);   //Send data
       
       payload = http.getString();                  //Get the response payload
       
       http.end();  //Close connection
     }

     payload.toCharArray(payLoad, POST_RESPONSE_PAYLOAD_SIZE);
     
     #ifdef DEBUG_SERIAL
      Serial.println("-------------- Response ---------------");
      Serial.println(payLoad);     
      Serial.println("----------------------------------------");
     #endif                  
         
     StaticJsonBuffer<200> jsonBuffer;
     JsonObject& root = jsonBuffer.parseObject(payLoad);

    if (!root.success())
    {
      Serial.println("ERROR JSON:  parseObject() failed");
      return ERROR_JSON_PARSE;
    }

     boolean success = root["success"];

     if(success == true) {
      return UPLOAD_SUCESS_TRUE;
     }
  return ERROR_SUCESS_FALSE;      
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

  uint8_t readEEPBfrPass[EEPROM_DATA_SIZE];
  getEEPROMBytes(readEEPBfrPass, PASSWORD_ADDR, EEPROM_DATA_SIZE);
  Serial.print("WiFi Password : ");
  String strPassword((char*)readEEPBfrPass);
//  Serial.println((const char*)readEEPBfr);
  Serial.println(strPassword);
  
  // Connect to Save SSID and Password
  WiFi.begin((const char *)readEEPBfr, (const char *)readEEPBfrPass);   //for WiFi connection

  uint8_t cntr = 0;
  while (WiFi.status() != WL_CONNECTED) {//Wait for the WiFI connection completion
    delay(1000);
    Serial.print("Waiting for connection with "); 
    Serial.println((const char *)readEEPBfr);
    cntr++;
    if(cntr > 15) break;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
     Serial.print("Connected to ");
     Serial.println(strSSID);      
  }
  Serial.println("------------ Ready ----------------");

  /*
   * Initialize SPACE for STACK like memory 
   * at EEPROM of saveing upload failed RFID tags 
   */  
   if(intWriteEEPROM(FAILED_TAG_STACK_PTR_ADDR, 0) == 0) {
    Serial.println("Initialized space for failed ID to store.");
   }  
}

uint8_t readBuffr[16];
uint8_t tagBuffr[6];
uint32_t timeTrack = 0;

void loop() {

  uint8_t getChar = Serial.read();
  uint8_t i = 0;
  bool flagPacketReadComplete = false;
   
  if(getChar == 0x02) {
    timeTrack = millis();    
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

    // ------------------ Ask for restart --------------------------------------
    Serial.print("Restart hardware module ? [Y]/[N]>");               
    charLen = serialReadline(readLineBuffr, EEPROM_DATA_SIZE, SERIAL_READ_TIMEOUT);
    for(uint8_t i = 0; i < charLen; i++)
      Serial.write(readLineBuffr[i]);
    Serial.write('\n');
    
    if(readLineBuffr[0] == 'Y' || readLineBuffr[0] == 'y') {
      ESP.restart();
    }
    
  } else {
    // Do nothing ----
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
    
    char jsonCharBuffer[POST_PAYLOAD_SIZE];
    encodeJson(jsonCharBuffer, POST_PAYLOAD_SIZE, readTag); // Create JSON 
    
    // post to server and handle if unsuccessfull    
    if(postToServer(url, jsonCharBuffer) > 1) {
      Serial.println("\nError updating database...\nUpdating later...");      
      if(addIDToEEPROMStack(readTag)) {
        char buffr[200];
        sprintf(buffr, "Saved tag %lu to EEPROM, will be sent later !", readTag);
        Serial.println(buffr);      
      } else {
        char buffr[200];
        sprintf(buffr, "Error updating tag \"%lu\" to EEPROM !", readTag);
        Serial.println(buffr);              
      }
    } else {
      // Do nothing if post is successfull !
    }
  }

  if((millis() - timeTrack) >= 30000) {        
    
    uint16_t stackPtr = intReadEEPROM(FAILED_TAG_STACK_PTR_ADDR);
    
    Serial.println("I ma here !!");
    Serial.print("Stack ptr : ");
    Serial.println(stackPtr);
    
    while(stackPtr) {         
                
        uint32_t missedID = getIDFromEEPROMStack();
        char jsonCharBuffer[POST_PAYLOAD_SIZE];
        
        encodeJson(jsonCharBuffer, POST_PAYLOAD_SIZE, missedID); // Create JSON 
        
        // post to server and handle if unsuccessfull    
        if(postToServer(url, jsonCharBuffer) > 1) {
          Serial.println("\nError updating database again ...\nUpdating later...");      
          if(addIDToEEPROMStack(missedID)) {
            char buffr[200];
            sprintf(buffr, "Saved tag %lu to EEPROM, will be sent later !", missedID);
            Serial.println(buffr);      
          }
        } else {
          char buffr[200];
          sprintf(buffr, "Updated tag \"%lu\" successfully !", missedID);
          Serial.println(buffr);      
        }
      if(Serial.available() > 0)  break;
      stackPtr = intReadEEPROM(FAILED_TAG_STACK_PTR_ADDR);
      delay(500);
    }
            
  }
}
