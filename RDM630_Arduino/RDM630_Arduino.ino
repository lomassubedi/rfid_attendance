#include "RFIDRdm630.h"
#include "Keypad.h"

#define     TIMEOUT     10000
// --------- Configuration of RFID Boards ---------
const int rxPin = 2;  // pin that will receive the data
const int txPin = 3;  // connection not necessary.

RFIDtag  tag;  // RFIDtag object

RFIDRdm630 reader = RFIDRdm630(rxPin,txPin);    // the reader object.

// ------ Configuration for 4X4 Keypad -------
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {7, 8, 9, 10}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {3, 4, 5, 6}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

// ------ Eng general configuration --------

#define   Q_SIZE  5
char quequ[Q_SIZE];
uint8_t quequPtr = 0;
char packet[100];
uint8_t pktIndx = 0;

void emptyArray(char *quequ, uint8_t ArySize) {
  while(ArySize--) {
    *quequ = '\0';
    quequ++;
  }
}

void setup() {
  Serial.begin(9600);   // open the Serial for show data
}


long rexIDVal = 0;
char keyStroke;
unsigned long startTime = 0;
bool flag_done_single_item = false;

void loop() {
  
  if (reader.isAvailable()){  // tests if a card was read by the module
    tag = reader.getTag();  // if true, then receives a tag object
    Serial.print(" Card Number: ");
    Serial.println(tag.getCardNumber());  // get cardNumber in long format
    Serial.println();
    rexIDVal = tag.getCardNumber();

    memcpy(packet, &rexIDVal, 4);
    packet[4] = ':';
    pktIndx = 4;

    keyStroke = customKeypad.getKey();
    
//    if(keyStroke) {
      // Start timer
//      startTime = millis();          
//      while((millis() - startTime) < TIMEOUT) {    
  
        if(keyStroke == '1' || keyStroke == '2' || \
            keyStroke == '3' || keyStroke == '4' || \
            keyStroke == '5' || keyStroke == '6' || \
            keyStroke == '7' || keyStroke == '8' || \
            keyStroke == '9') {
              quequ[0] = keyStroke;            
              if(keyStroke == '1') {
                unsigned long startSecDigitTime = millis();
                while((millis() - startSecDigitTime) < 5000) {
                 keyStroke = customKeypad.getKey();  
                 if(keyStroke == '1' || keyStroke == '2' || \
                    keyStroke == '3' || keyStroke == '4' || \
                    keyStroke == '5') {
                      quequ[1] = keyStroke;                  
                      flag_done_single_item = true;
                      break;
                    }
                }
              } else {
                flag_done_single_item = true;
              }
            } else if(keyStroke == 'A' && flag_done_single_item) {
              Serial.println(quequ);
              flag_done_single_item = false;
              emptyArray(quequ, Q_SIZE);
            }
  
            /*
            if(flag_done_single_item) {
              Serial.println(quequ);
              flag_done_single_item = false;
            } */
              /*
            } else if(keyStroke == 'A') {
              // Nothing to do here !!
              memcpy(&packet[pktIndx], quequ, quequPtr);
              packet[pktIndx++] = ',';
              pktIndx += quequPtr;
              emptyQueue();
            } else if(keyStroke == 'B') {
              // Empty Queue
              emptyQueue();
              // empty packet
              pktIndx = 4;
            } else if(keyStroke == 'D') {
              // Create a packet and send
              if(pktIndx > 4) {
                Serial.println(packet);
                pktIndx = 4;
              }
            } else {
              // Do nothing 
            }
            */
//      }
  //     emptyQueue();
//    }
      
  }
}
