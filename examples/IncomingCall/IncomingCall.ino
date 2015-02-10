// CELLSTICK Incoming Call Number Example
// Listens for a call and displays the phone number of the caller (if available).
// Use this example to add phone call detection to your own CELLSTICK sketch.

#include <SoftwareSerial.h>
#include "Tinkeringtech_CELLSTICK.h"

// Pins which are connected to the CELLSTICK.
// Note that this is different from CELLSTICKtest!
#define CELLSTICK_RX            2
#define CELLSTICK_TX            3
#define CELLSTICK_RST           4

// Note you need to map interrupt number to pin number
// for your board.  On an Uno & Mega interrupt 0 is
// digital pin 2, and on a Leonardo interrupt 0 is
// digital pin 3.  See this page for a complete table:
//   http://arduino.cc/en/Reference/attachInterrupt
// Make sure this interrupt pin is connected to CELLSTICK RI!
#define CELLSTICK_RI_INTERRUPT  0
                             
SoftwareSerial cellstickSS = SoftwareSerial(CELLSTICK_TX, CELLSTICK_RX);
Tinkeringtech_CELLSTICK cellstick = Tinkeringtech_CELLSTICK(&cellstickSS, CELLSTICK_RST);

void setup() {
  Serial.begin(115200);
  Serial.println(F("CELLSTICK incoming call example"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  // Initialize CELLSTICK.
  if (! cellstick.begin(4800)) {  // make it slow so its easy to read!
    Serial.println(F("Couldn't find CELLSTICK"));
    while (1);
  }
  Serial.println(F("CELLSTICK is OK"));
  
  // Enable incoming call notification.
  if(cellstick.callerIdNotification(true, CELLSTICK_RI_INTERRUPT)) {
    Serial.println(F("Caller id notification enabled."));
  }
  else {
    Serial.println(F("Caller id notification disabled"));
  }
}

void loop(){
  // Create a small string buffer to hold incoming call number.
  char phone[32] = {0};
  // Check for an incoming call.  Will return true if a call is incoming.
  if(cellstick.incomingCallNumber(phone)){
    Serial.println(F("RING!"));
    Serial.print(F("Phone Number: "));
    Serial.println(phone);
  }
}