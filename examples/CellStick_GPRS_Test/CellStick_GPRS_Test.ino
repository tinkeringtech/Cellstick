/***************************************************
  This is a test program to test the GPRS on the TinkeringTech CellStick. GPRS is turned on, a webpage read, followed by GPRS turned off.
  Insert the SIM, battery and antenna on the CellStick and upload this sketch.
  Hold down the the Power key and the BLUE Cellular Netstat light should start blinking.
  Open the Serial terminal at 115200 baud and the sketch will turn on GPRS, connect to a webpage, and then turn off GPRS.
  It may take a few iterations of the loop to get the GPRS turned on.

  This library is based on the Adafruit FONA library written by Limor Fried/Ladyada for Adafruit Industries. Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing products from Adafruit and Tinkeringtech.
  
  www.adafruit.com
  www.tinkeringtech.com
  BSD license, all text above must be included in any redistribution
 ****************************************************/

/* 
THIS CODE IS STILL IN PROGRESS!

Open up the serial console on the Arduino at 115200 baud to interact with CELLSTICK

*/

#include <SoftwareSerial.h>
#include "Tinkeringtech_CELLSTICK.h"

#define CELLSTICK_RX 2
#define CELLSTICK_TX 3
#define CELLSTICK_RST 4

// this is a large buffer for replies
char replybuffer[255];
String test;

SoftwareSerial cellstickSS = SoftwareSerial(CELLSTICK_TX, CELLSTICK_RX);
Tinkeringtech_CELLSTICK cellstick = Tinkeringtech_CELLSTICK(&cellstickSS, CELLSTICK_RST);

//uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

void setup() {
  Serial.begin(115200);
  Serial.println(F("CELLSTICK GPRS test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  
  // See if the CELLSTICK is responding
  if (! cellstick.begin(4800)) {  // make it slow so its easy to read!
    Serial.println(F("Couldn't find CELLSTICK"));
    while (1);
  }
  Serial.println(F("CELLSTICK is OK"));

  // Print SIM card IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = cellstick.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("SIM card IMEI: "); Serial.println(imei);
  }


}

void loop() {
      // turn GPRS on
      Serial.println(F("****"));
      cellstick.enableGPRS(true);
      Serial.println(F("Turn on GPRS..waiting 10 seconds to turn on"));
      delay(10000);

      // read website URL
      uint16_t statuscode;
      int16_t length;
      char url[80];
      
      flushSerial();
      Serial.println(F("read a small webpage"));
      Serial.println(F("URL to read (e.g. www.adafruit.com/testwifi/index.html):"));
      test="http://www.adafruit.com/testwifi/index.html";
      test.toCharArray(url, 101);
      Serial.println(url);
       if (!cellstick.HTTP_GET_start(url, &statuscode, (uint16_t *)&length)) {
         Serial.println("Failed!");
        // break;
       }
       while (length > 0) {
         while (cellstick.available()) {
           char c = cellstick.read();
           
           // Serial.write is too slow, we'll write directly to Serial register!
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
           loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
           UDR0 = c;
#else
           Serial.write(c);
#endif
           length--;
           if (! length) ;
         }
       }
       
       cellstick.HTTP_GET_end();
       
      //Turn off GPRS
       cellstick.enableGPRS(false);
       Serial.println(F("Turn off GPRS"));
       Serial.println(F("\n****"));

}

void flushSerial() {
    while (Serial.available()) 
    Serial.read();
}
