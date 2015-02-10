/***************************************************
  This is a library for our Adafruit FONA Cellular Module

  Designed specifically to work with the Adafruit FONA
  ----> http://www.adafruit.com/products/1946
  ----> http://www.adafruit.com/products/1963

  These displays use TTL Serial to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#include <avr/pgmspace.h>
    // next line per http://postwarrior.com/arduino-ethershield-error-prog_char-does-not-name-a-type/
#define prog_char  char PROGMEM

#if (ARDUINO >= 100)
 #include "Arduino.h"
 #include <SoftwareSerial.h>
#else
 #include "WProgram.h"
 #include <NewSoftSerial.h>
#endif

#include "Tinkeringtech_CELLSTICK.h"

#if ARDUINO >= 100
Tinkeringtech_CELLSTICK::Tinkeringtech_CELLSTICK(SoftwareSerial *ss, int8_t rst)
#else
Tinkeringtech_CELLSTICK::Tinkeringtech_CELLSTICK(NewSoftSerial *ssm, int8_t rst)
#endif
{
  _rstpin = rst;
  mySerial = ss;
  apn = F("CELLSTICKnet");
  apnusername = 0;
  apnpassword = 0;
  httpsredirect = false;
  useragent = F("CELLSTICK");
}

boolean Tinkeringtech_CELLSTICK::begin(uint16_t baudrate) {
  pinMode(_rstpin, OUTPUT);
  digitalWrite(_rstpin, HIGH);
  delay(10);
  digitalWrite(_rstpin, LOW);
  delay(100);
  digitalWrite(_rstpin, HIGH);

  // give 3 seconds to reboot
  delay(3000);

  mySerial->begin(baudrate);
  delay(500);
  while (mySerial->available()) mySerial->read();

  sendCheckReply(F("AT"), F("OK"));
  delay(100);
  sendCheckReply(F("AT"), F("OK"));
  delay(100);
  sendCheckReply(F("AT"), F("OK"));
  delay(100);

  // turn off Echo!
  sendCheckReply(F("ATE0"), F("OK"));
  delay(100);

  if (! sendCheckReply(F("ATE0"), F("OK"))) {
    return false;
  }

  return true;
}



/********* BATTERY & ADC ********************************************/

/* returns value in mV (uint16_t) */
boolean Tinkeringtech_CELLSTICK::getBattVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CBC"), F("+CBC: "), v, ',', 2);
}

/* returns the percentage charge of battery as reported by sim800 */
boolean Tinkeringtech_CELLSTICK::getBattPercent(uint16_t *p) {
  return sendParseReply(F("AT+CBC"), F("+CBC: "), p, ',', 1);
}

boolean Tinkeringtech_CELLSTICK::getADCVoltage(uint16_t *v) {
  return sendParseReply(F("AT+CADC?"), F("+CADC: 1,"), v);
}

/********* SIM ***********************************************************/

uint8_t Tinkeringtech_CELLSTICK::unlockSIM(char *pin)
{
  char sendbuff[14] = "AT+CPIN=";
  sendbuff[8] = pin[0];
  sendbuff[9] = pin[1];
  sendbuff[10] = pin[2];
  sendbuff[11] = pin[3];
  sendbuff[12] = NULL;

  return sendCheckReply(sendbuff, "OK");
}

uint8_t Tinkeringtech_CELLSTICK::getSIMCCID(char *ccid) {
  getReply("AT+CCID");
  // up to 20 chars
  strncpy(ccid, replybuffer, 20);
  ccid[20] = 0;

  readline(); // eat 'OK'

  return strlen(ccid);
}

/********* IMEI **********************************************************/

uint8_t Tinkeringtech_CELLSTICK::getIMEI(char *imei) {
  getReply("AT+GSN");

  // up to 15 chars
  strncpy(imei, replybuffer, 15);
  imei[15] = 0;

  readline(); // eat 'OK'

  return strlen(imei);
}

/********* NETWORK *******************************************************/

uint8_t Tinkeringtech_CELLSTICK::getNetworkStatus(void) {
  uint16_t status;

  if (! sendParseReply(F("AT+CREG?"), F("+CREG: "), &status, ',', 1)) return 0;

  return status;
}


uint8_t Tinkeringtech_CELLSTICK::getRSSI(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CSQ"), F("+CSQ: "), &reply) ) return 0;

  return reply;
}

/********* AUDIO *******************************************************/

boolean Tinkeringtech_CELLSTICK::setAudio(uint8_t a) {
  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  return sendCheckReply(F("AT+CHFA="), a, F("OK"));
}

uint8_t Tinkeringtech_CELLSTICK::getVolume(void) {
  uint16_t reply;

  if (! sendParseReply(F("AT+CLVL?"), F("+CLVL: "), &reply) ) return 0;

  return reply;
}

boolean Tinkeringtech_CELLSTICK::setVolume(uint8_t i) {
  return sendCheckReply(F("AT+CLVL="), i, F("OK"));
}


boolean Tinkeringtech_CELLSTICK::playToolkitTone(uint8_t t, uint16_t len) {
  return sendCheckReply(F("AT+STTONE=1,"), t, len, F("OK"));
}

boolean Tinkeringtech_CELLSTICK::setMicVolume(uint8_t a, uint8_t level) {
  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  return sendCheckReply(F("AT+CMIC="), a, level, F("OK"));
}

/********* FM RADIO *******************************************************/


boolean Tinkeringtech_CELLSTICK::FMradio(boolean onoff, uint8_t a) {
  if (! onoff) {
    return sendCheckReply(F("AT+FMCLOSE"), F("OK"));
  }

  // 0 is headset, 1 is external audio
  if (a > 1) return false;

  return sendCheckReply(F("AT+FMOPEN="), a, F("OK"));
}

boolean Tinkeringtech_CELLSTICK::tuneFMradio(uint16_t station) {
  // Fail if FM station is outside allowed range.
  if ((station < 870) || (station > 1090))
    return false;

  return sendCheckReply(F("AT+FMFREQ="), station, F("OK"));
}

boolean Tinkeringtech_CELLSTICK::setFMVolume(uint8_t i) {
  // Fail if volume is outside allowed range (0-6).
  if (i > 6) {
    return false;
  }
  // Send FM volume command and verify response.
  return sendCheckReply(F("AT+FMVOLUME="), i, F("OK"));
}

int8_t Tinkeringtech_CELLSTICK::getFMVolume() {
  uint16_t level;

  if (! sendParseReply(F("AT+FMVOLUME?"), F("+FMVOLUME: "), &level) ) return 0;

  return level;
}

int8_t Tinkeringtech_CELLSTICK::getFMSignalLevel(uint16_t station) {
  // Fail if FM station is outside allowed range.
  if ((station < 875) || (station > 1080)) {
    return -1;
  }

  // Send FM signal level query command.
  // Note, need to explicitly send timeout so right overload is chosen.
  getReply(F("AT+FMSIGNAL="), station, CELLSTICK_DEFAULT_TIMEOUT_MS);
  // Check response starts with expected value.
  char *p = strstr_P(replybuffer, PSTR("+FMSIGNAL: "));
  if (p == 0) return -1;
  p+=11;
  // Find second colon to get start of signal quality.
  p = strchr(p, ':');
  if (p == 0) return -1;
  p+=1;
  // Parse signal quality.
  int8_t level = atoi(p);
  readline();  // eat the "OK"
  return level;
}

/********* PWM/BUZZER **************************************************/

boolean Tinkeringtech_CELLSTICK::PWM(uint16_t period, uint8_t duty) {
  if (period > 2000) return false;
  if (duty > 100) return false;

  return sendCheckReply(F("AT+SPWM=0,"), period, duty, F("OK"));
}

/********* CALL PHONES **************************************************/
boolean Tinkeringtech_CELLSTICK::callPhone(char *number) {
  char sendbuff[35] = "ATD";
  strncpy(sendbuff+3, number, min(30, strlen(number)));
  uint8_t x = strlen(sendbuff);
  sendbuff[x] = ';';
  sendbuff[x+1] = 0;
  //Serial.println(sendbuff);

  return sendCheckReply(sendbuff, "OK");
}

boolean Tinkeringtech_CELLSTICK::hangUp(void) {
  return sendCheckReply(F("ATH0"), F("OK"));
}

boolean Tinkeringtech_CELLSTICK::pickUp(void) {
  return sendCheckReply(F("ATA"), F("OK"));
}

void Tinkeringtech_CELLSTICK::onIncomingCall() {
  #ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.print(F("> ")); Serial.println(F("Incoming call..."));
  #endif
  Tinkeringtech_CELLSTICK::_incomingCall = true;
}

boolean Tinkeringtech_CELLSTICK::_incomingCall = false;

boolean Tinkeringtech_CELLSTICK::callerIdNotification(boolean enable, uint8_t interrupt) {
  if(enable){
    attachInterrupt(interrupt, onIncomingCall, FALLING);
    return sendCheckReply(F("AT+CLIP=1"), F("OK"));
  }

  detachInterrupt(interrupt);
  return sendCheckReply(F("AT+CLIP=0"), F("OK"));
}

boolean Tinkeringtech_CELLSTICK::incomingCallNumber(char* phonenum) {
  //+CLIP: "<incoming phone number>",145,"",0,"",0
  if(!Tinkeringtech_CELLSTICK::_incomingCall)
    return false;

  readline();
  while(!strcmp_P(replybuffer, (prog_char*)F("RING")) == 0) {
    flushInput();
    readline();
  }

  readline(); //reads incoming phone number line

  parseReply(F("+CLIP: \""), phonenum, '"');

  #ifdef Tinkeringtech_CELLSTICK_DEBUG
    Serial.print(F("Phone Number: "));
    Serial.println(replybuffer);
  #endif

  Tinkeringtech_CELLSTICK::_incomingCall = false;
  return true;
}

/********* SMS **********************************************************/

int8_t Tinkeringtech_CELLSTICK::getNumSMS(void) {
  uint16_t numsms;

  if (! sendCheckReply(F("AT+CMGF=1"), F("OK"))) return -1;
  // ask how many sms are stored

  if (! sendParseReply(F("AT+CPMS?"), F("+CPMS: \"SM_P\","), &numsms) ) return -1;

  return numsms;
}

// Reading SMS's is a bit involved so we don't use helpers that may cause delays or debug
// printouts!
boolean Tinkeringtech_CELLSTICK::readSMS(uint8_t i, char *smsbuff, 
			       uint16_t maxlen, uint16_t *readlen) {
  // text mode
  if (! sendCheckReply(F("AT+CMGF=1"), F("OK"))) return false;

  // show all text mode parameters
  if (! sendCheckReply(F("AT+CSDH=1"), F("OK"))) return false;

  // parse out the SMS len
  uint16_t thesmslen = 0;

  //getReply(F("AT+CMGR="), i, 1000);  //  do not print debug!
  mySerial->print(F("AT+CMGR="));
  mySerial->println(i);
  readline(1000); // timeout

  // parse it out...
  parseReply(F("+CMGR:"), &thesmslen, ',', 11);

  readRaw(thesmslen);

  flushInput();

  uint16_t thelen = min(maxlen, strlen(replybuffer));
  strncpy(smsbuff, replybuffer, thelen);
  smsbuff[thelen] = 0; // end the string

#ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.println(replybuffer);
#endif
  *readlen = thelen;
  return true;
}

// Retrieve the sender of the specified SMS message and copy it as a string to
// the sender buffer.  Up to senderlen characters of the sender will be copied
// and a null terminator will be added if less than senderlen charactesr are
// copied to the result.  Returns true if a result was successfully retrieved,
// otherwise false.
boolean Tinkeringtech_CELLSTICK::getSMSSender(uint8_t i, char *sender, int senderlen) {
  // Ensure text mode and all text mode parameters are sent.
  if (! sendCheckReply(F("AT+CMGF=1"), F("OK"))) return false;
  if (! sendCheckReply(F("AT+CSDH=1"), F("OK"))) return false;
  // Send command to retrieve SMS message and parse a line of response.
  mySerial->print(F("AT+CMGR="));
  mySerial->println(i);
  readline(1000);
  // Parse the second field in the response.
  boolean result = parseReplyQuoted(F("+CMGR:"), sender, senderlen, ',', 1);
  // Drop any remaining data from the response.
  flushInput();
  return result;
}

boolean Tinkeringtech_CELLSTICK::sendSMS(char *smsaddr, char *smsmsg) {
  if (! sendCheckReply("AT+CMGF=1", "OK")) return -1;

  char sendcmd[30] = "AT+CMGS=\"";
  strncpy(sendcmd+9, smsaddr, 30-9-2);  // 9 bytes beginning, 2 bytes for close quote + null
  sendcmd[strlen(sendcmd)] = '\"';

  if (! sendCheckReply(sendcmd, "> ")) return false;
#ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.print("> "); Serial.println(smsmsg);
#endif
  mySerial->println(smsmsg);
  mySerial->println();
  mySerial->write(0x1A);
#ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.println("^Z");
#endif
  readline(10000); // read the +CMGS reply, wait up to 10 seconds!!!
  //Serial.print("* "); Serial.println(replybuffer);
  if (strstr(replybuffer, "+CMGS") == 0) {
    return false;
  }
  readline(1000); // read OK
  //Serial.print("* "); Serial.println(replybuffer);

  if (strcmp(replybuffer, "OK") != 0) {
    return false;
  }

  return true;
}


boolean Tinkeringtech_CELLSTICK::deleteSMS(uint8_t i) {
    if (! sendCheckReply("AT+CMGF=1", "OK")) return -1;
  // read an sms
  char sendbuff[12] = "AT+CMGD=000";
  sendbuff[8] = (i / 100) + '0';
  i %= 100;
  sendbuff[9] = (i / 10) + '0';
  i %= 10;
  sendbuff[10] = i + '0';

  return sendCheckReply(sendbuff, "OK", 2000);
}

/********* TIME **********************************************************/

boolean Tinkeringtech_CELLSTICK::enableNetworkTimeSync(boolean onoff) {
  if (onoff) {
    if (! sendCheckReply(F("AT+CLTS=1"), F("OK")))
      return false;
  } else {
    if (! sendCheckReply(F("AT+CLTS=0"), F("OK")))
      return false;
  }

  flushInput(); // eat any 'Unsolicted Result Code'

  return true;
}

boolean Tinkeringtech_CELLSTICK::enableNTPTimeSync(boolean onoff, const __FlashStringHelper *ntpserver) {
  if (onoff) {
    if (! sendCheckReply(F("AT+CNTPCID=1"), F("OK")))
      return false;

    mySerial->print(F("AT+CNTP=\""));
    if (ntpserver != 0) {
      mySerial->print(ntpserver);
    } else {
      mySerial->print(F("pool.ntp.org"));
    }
    mySerial->println(F("\",0"));
    readline(CELLSTICK_DEFAULT_TIMEOUT_MS);
    if (strcmp(replybuffer, "OK") != 0)
      return false;

    if (! sendCheckReply(F("AT+CNTP"), F("OK"), 10000))
      return false;

    uint16_t status;
    readline(10000);
    if (! parseReply(F("+CNTP:"), &status))
      return false;
  } else {
    if (! sendCheckReply(F("AT+CNTPCID=0"), F("OK")))
      return false;
  }

  return true;
}

boolean Tinkeringtech_CELLSTICK::getTime(char *buff, uint16_t maxlen) {
  getReply(F("AT+CCLK?"), (uint16_t) 10000);
  if (strncmp(replybuffer, "+CCLK: ", 7) != 0)
    return false;

  char *p = replybuffer+7;
  uint16_t lentocopy = min(maxlen-1, strlen(p));
  strncpy(buff, p, lentocopy+1);
  buff[lentocopy] = 0;

  readline(); // eat OK

  return true;
}

/********* GPRS **********************************************************/

boolean Tinkeringtech_CELLSTICK::enableGPRS(boolean onoff) {

  if (onoff) {
    if (! sendCheckReply(F("AT+CGATT=1"), F("OK"), 10000))
      return false;

    // set bearer profile! connection type GPRS
    if (! sendCheckReply(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""),
			   F("OK"), 10000))
      return false;

    // set bearer profile access point name
    if (apn) {
      // Send command AT+SAPBR=3,1,"APN","<apn value>" where <apn value> is the configured APN value.
      if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"APN\","), apn, F("OK"), 10000))
        return false;

      // set username/password
      if (apnusername) {
        // Send command AT+SAPBR=3,1,"USER","<user>" where <user> is the configured APN username.
        if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"USER\","), apnusername, F("OK"), 10000))
          return false;
      }
      if (apnpassword) {
        // Send command AT+SAPBR=3,1,"PWD","<password>" where <password> is the configured APN password.
        if (! sendCheckReplyQuoted(F("AT+SAPBR=3,1,\"PWD\","), apnpassword, F("OK"), 10000))
          return false;
      }
    }

    // open GPRS context
    if (! sendCheckReply(F("AT+SAPBR=1,1"), F("OK"), 10000))
      return false;
  } else {
    // close GPRS context
    if (! sendCheckReply(F("AT+SAPBR=0,1"), F("OK"), 10000))
      return false;

    if (! sendCheckReply(F("AT+CGATT=0"), F("OK"), 10000))
      return false;

  }
  return true;
}

uint8_t Tinkeringtech_CELLSTICK::GPRSstate(void) {
  uint16_t state;

  if (! sendParseReply(F("AT+CGATT?"), F("+AT+CGATT: "), &state) )
    return -1;

  return state;
}

void Tinkeringtech_CELLSTICK::setGPRSNetworkSettings(const __FlashStringHelper *apn,
              const __FlashStringHelper *username, const __FlashStringHelper *password) {
  this->apn = apn;
  this->apnusername = username;
  this->apnpassword = password;
}

boolean Tinkeringtech_CELLSTICK::getGSMLoc(uint16_t *errorcode, char *buff, uint16_t maxlen) {

  getReply(F("AT+CIPGSMLOC=1,1"), (uint16_t)10000);

  if (! parseReply(F("+CIPGSMLOC: "), errorcode))
    return false;

  char *p = replybuffer+14;
  uint16_t lentocopy = min(maxlen-1, strlen(p));
  strncpy(buff, p, lentocopy+1);

  readline(); // eat OK

  return true;
}

boolean Tinkeringtech_CELLSTICK::HTTP_GET_start(char *url,
              uint16_t *status, uint16_t *datalen){
  if (! HTTP_initialize(url))
    return false;

  // HTTP GET
  if (! sendCheckReply(F("AT+HTTPACTION=0"), F("OK")))
    return false;

  // HTTP response
  if (! HTTP_response(status, datalen))
    return false;

  return true;
}

void Tinkeringtech_CELLSTICK::HTTP_GET_end(void) {
  HTTP_terminate();
}

boolean Tinkeringtech_CELLSTICK::HTTP_POST_start(char *url,
              const __FlashStringHelper *contenttype,
              const uint8_t *postdata, uint16_t postdatalen,
              uint16_t *status, uint16_t *datalen){
  if (! HTTP_initialize(url))
    return false;

  if (! sendCheckReplyQuoted(F("AT+HTTPPARA=\"CONTENT\","), contenttype, F("OK"), 10000))
    return false;

  // HTTP POST data
  flushInput();
  mySerial->print(F("AT+HTTPDATA="));
  mySerial->print(postdatalen);
  mySerial->println(",10000");
  readline(CELLSTICK_DEFAULT_TIMEOUT_MS);
  if (strcmp(replybuffer, "DOWNLOAD") != 0)
    return false;

  mySerial->write(postdata, postdatalen);
  readline(10000);
  if (strcmp(replybuffer, "OK") != 0)
    return false;

  // HTTP POST
  if (! sendCheckReply(F("AT+HTTPACTION=1"), F("OK")))
    return false;

  if (! HTTP_response(status, datalen))
    return false;

  return true;
}

void Tinkeringtech_CELLSTICK::HTTP_POST_end(void) {
  HTTP_terminate();
}

void Tinkeringtech_CELLSTICK::setUserAgent(const __FlashStringHelper *useragent) {
  this->useragent = useragent;
}

void Tinkeringtech_CELLSTICK::setHTTPSRedirect(boolean onoff) {
  httpsredirect = onoff;
}

/********* HTTP HELPERS ****************************************/

boolean Tinkeringtech_CELLSTICK::HTTP_initialize(char *url) {
  // Handle any pending
  HTTP_terminate();

  // Initialize and set parameters
  if (! sendCheckReply(F("AT+HTTPINIT"), F("OK")))
    return false;
  if (! sendCheckReply(F("AT+HTTPPARA=\"CID\",1"), F("OK")))
    return false;
  if (! sendCheckReplyQuoted(F("AT+HTTPPARA=\"UA\","), useragent, F("OK"), 10000))
    return false;

  flushInput();
  mySerial->print(F("AT+HTTPPARA=\"URL\",\""));
  mySerial->print(url);
  mySerial->println("\"");
  readline(CELLSTICK_DEFAULT_TIMEOUT_MS);
  if (strcmp(replybuffer, "OK") != 0)
    return false;

  // HTTPS redirect
  if (httpsredirect) {
    if (! sendCheckReply(F("AT+HTTPPARA=\"REDIR\",1"), F("OK")))
      return false;

    if (! sendCheckReply(F("AT+HTTPSSL=1"), F("OK")))
      return false;
  }

  return true;
}

boolean Tinkeringtech_CELLSTICK::HTTP_response(uint16_t *status, uint16_t *datalen) {
  // Read response status
  readline(10000);

  if (! parseReply(F("+HTTPACTION:"), status, ',', 1))
    return false;
  if (! parseReply(F("+HTTPACTION:"), datalen, ',', 2))
    return false;

  Serial.print("Status: "); Serial.println(*status);
  Serial.print("Len: "); Serial.println(*datalen);

  // Read response
  getReply(F("AT+HTTPREAD"));

  return true;
}

void Tinkeringtech_CELLSTICK::HTTP_terminate(void) {
  flushInput();
  sendCheckReply(F("AT+HTTPTERM"), F("OK"));
}

/********* LOW LEVEL *******************************************/

inline int Tinkeringtech_CELLSTICK::available(void) {
  return mySerial->available();
}

inline size_t Tinkeringtech_CELLSTICK::write(uint8_t x) {
  return mySerial->write(x);
}

inline int Tinkeringtech_CELLSTICK::read(void) {
  return mySerial->read();
}

inline int Tinkeringtech_CELLSTICK::peek(void) {
  return mySerial->peek();
}

inline void Tinkeringtech_CELLSTICK::flush() {
  mySerial->flush();
}

void Tinkeringtech_CELLSTICK::flushInput() {
  // Read all available serial input to flush pending data.
  while(available()) {
     read();
  }
}

uint16_t Tinkeringtech_CELLSTICK::readRaw(uint16_t b) {
  uint16_t idx = 0;

  while (b && (idx < sizeof(replybuffer)-1)) {
    if (mySerial->available()) {
      replybuffer[idx] = mySerial->read();
      idx++;
      b--;
    }
  }
  replybuffer[idx] = 0;

  return idx;
}

uint8_t Tinkeringtech_CELLSTICK::readline(uint16_t timeout, boolean multiline) {
  uint16_t replyidx = 0;

  while (timeout--) {
    if (replyidx >= 254) {
      //Serial.println(F("SPACE"));
      break;
    }

    while(mySerial->available()) {
      char c =  mySerial->read();
      if (c == '\r') continue;
      if (c == 0xA) {
        if (replyidx == 0)   // the first 0x0A is ignored
          continue;

        if (!multiline) {
          timeout = 0;         // the second 0x0A is the end of the line
          break;
        }
      }
      replybuffer[replyidx] = c;
      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);
      replyidx++;
    }

    if (timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  replybuffer[replyidx] = 0;  // null term
  return replyidx;
}

uint8_t Tinkeringtech_CELLSTICK::getReply(char *send, uint16_t timeout) {
  flushInput();

#ifdef Tinkeringtech_CELLSTICK_DEBUG
    Serial.print("\t---> "); Serial.println(send);
#endif

  mySerial->println(send);

  uint8_t l = readline(timeout);
#ifdef Tinkeringtech_CELLSTICK_DEBUG
    Serial.print ("\t<--- "); Serial.println(replybuffer);
#endif
  return l;
}

uint8_t Tinkeringtech_CELLSTICK::getReply(const __FlashStringHelper *send, uint16_t timeout) {
  flushInput();

#ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.print("\t---> "); Serial.println(send);
#endif

  mySerial->println(send);

  uint8_t l = readline(timeout);
#ifdef Tinkeringtech_CELLSTICK_DEBUG
    Serial.print ("\t<--- "); Serial.println(replybuffer);
#endif
  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Tinkeringtech_CELLSTICK::getReply(const __FlashStringHelper *prefix, char *suffix, uint16_t timeout) {
  flushInput();

#ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.print("\t---> "); Serial.print(prefix); Serial.println(suffix);
#endif

  mySerial->print(prefix);
  mySerial->println(suffix);

  uint8_t l = readline(timeout);
#ifdef Tinkeringtech_CELLSTICK_DEBUG
    Serial.print ("\t<--- "); Serial.println(replybuffer);
#endif
  return l;
}

// Send prefix, suffix, and newline. Return response (and also set replybuffer with response).
uint8_t Tinkeringtech_CELLSTICK::getReply(const __FlashStringHelper *prefix, int32_t suffix, uint16_t timeout) {
  flushInput();

#ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.print("\t---> "); Serial.print(prefix); Serial.println(suffix, DEC);
#endif

  mySerial->print(prefix);
  mySerial->println(suffix, DEC);

  uint8_t l = readline(timeout);
#ifdef Tinkeringtech_CELLSTICK_DEBUG
    Serial.print ("\t<--- "); Serial.println(replybuffer);
#endif
  return l;
}

// Send prefix, suffix, suffix2, and newline. Return response (and also set replybuffer with response).
uint8_t Tinkeringtech_CELLSTICK::getReply(const __FlashStringHelper *prefix, int32_t suffix1, int32_t suffix2, uint16_t timeout) {
  flushInput();

#ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.print("\t---> "); Serial.print(prefix);
  Serial.print(suffix1, DEC); Serial.print(","); Serial.println(suffix2, DEC);
#endif

  mySerial->print(prefix);
  mySerial->print(suffix1);
  mySerial->print(',');
  mySerial->println(suffix2, DEC);

  uint8_t l = readline(timeout);
#ifdef Tinkeringtech_CELLSTICK_DEBUG
    Serial.print ("\t<--- "); Serial.println(replybuffer);
#endif
  return l;
}

// Send prefix, ", suffix, ", and newline. Return response (and also set replybuffer with response).
uint8_t Tinkeringtech_CELLSTICK::getReplyQuoted(const __FlashStringHelper *prefix, const __FlashStringHelper *suffix, uint16_t timeout) {
  flushInput();

#ifdef Tinkeringtech_CELLSTICK_DEBUG
  Serial.print("\t---> "); Serial.print(prefix);
  Serial.print('"'); Serial.print(suffix); Serial.println('"');
#endif

  mySerial->print(prefix);
  mySerial->print('"');
  mySerial->print(suffix);
  mySerial->println('"');

  uint8_t l = readline(timeout);
#ifdef Tinkeringtech_CELLSTICK_DEBUG
    Serial.print ("\t<--- "); Serial.println(replybuffer);
#endif
  return l;
}

boolean Tinkeringtech_CELLSTICK::sendCheckReply(char *send, char *reply, uint16_t timeout) {
  getReply(send, timeout);

/*
  for (uint8_t i=0; i<strlen(replybuffer); i++) {
    Serial.print(replybuffer[i], HEX); Serial.print(" ");
  }
  Serial.println();
  for (uint8_t i=0; i<strlen(reply); i++) {
    Serial.print(reply[i], HEX); Serial.print(" ");
  }
  Serial.println();
  */
  return (strcmp(replybuffer, reply) == 0);
}

boolean Tinkeringtech_CELLSTICK::sendCheckReply(const __FlashStringHelper *send, const __FlashStringHelper *reply, uint16_t timeout) {
  getReply(send, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify CELLSTICK response matches reply parameter.
boolean Tinkeringtech_CELLSTICK::sendCheckReply(const __FlashStringHelper *prefix, char *suffix, const __FlashStringHelper *reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, and newline.  Verify CELLSTICK response matches reply parameter.
boolean Tinkeringtech_CELLSTICK::sendCheckReply(const __FlashStringHelper *prefix, int32_t suffix, const __FlashStringHelper *reply, uint16_t timeout) {
  getReply(prefix, suffix, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, suffix, suffix2, and newline.  Verify CELLSTICK response matches reply parameter.
boolean Tinkeringtech_CELLSTICK::sendCheckReply(const __FlashStringHelper *prefix, int32_t suffix1, int32_t suffix2, const __FlashStringHelper *reply, uint16_t timeout) {
  getReply(prefix, suffix1, suffix2, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}

// Send prefix, ", suffix, ", and newline.  Verify CELLSTICK response matches reply parameter.
boolean Tinkeringtech_CELLSTICK::sendCheckReplyQuoted(const __FlashStringHelper *prefix, const __FlashStringHelper *suffix, const __FlashStringHelper *reply, uint16_t timeout) {
  getReplyQuoted(prefix, suffix, timeout);
  return (strcmp_P(replybuffer, (prog_char*)reply) == 0);
}


boolean Tinkeringtech_CELLSTICK::parseReply(const __FlashStringHelper *toreply,
          uint16_t *v, char divider, uint8_t index) {
  char *p = strstr_P(replybuffer, (prog_char*)toreply);  // get the pointer to the voltage
  if (p == 0) return false;
  p+=strlen_P((prog_char*)toreply);
  //Serial.println(p);
  for (uint8_t i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
    //Serial.println(p);

  }
  *v = atoi(p);

  return true;
}

boolean Tinkeringtech_CELLSTICK::parseReply(const __FlashStringHelper *toreply,
          char *v, char divider, uint8_t index) {
  uint8_t i=0;
  char *p = strstr_P(replybuffer, (prog_char*)toreply);
  if (p == 0) return false;
  p+=strlen_P((prog_char*)toreply);

  for (i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
  }

  for(i=0; i<strlen(p);i++) {
    if(p[i] == divider)
      break;
    v[i] = p[i];
  }

  v[i] = '\0';

  return true;
}

// Parse a quoted string in the response fields and copy its value (without quotes)
// to the specified character array (v).  Only up to maxlen characters are copied
// into the result buffer, so make sure to pass a large enough buffer to handle the
// response.
boolean Tinkeringtech_CELLSTICK::parseReplyQuoted(const __FlashStringHelper *toreply,
          char *v, int maxlen, char divider, uint8_t index) {
  uint8_t i=0, j;
  // Verify response starts with toreply.
  char *p = strstr_P(replybuffer, (prog_char*)toreply);
  if (p == 0) return false;
  p+=strlen_P((prog_char*)toreply);

  // Find location of desired response field.
  for (i=0; i<index;i++) {
    // increment dividers
    p = strchr(p, divider);
    if (!p) return false;
    p++;
  }

  // Copy characters from response field into result string.
  for(i=0, j=0; j<maxlen && i<strlen(p); ++i) {
    // Stop if a divier is found.
    if(p[i] == divider)
      break;
    // Skip any quotation marks.
    else if(p[i] == '"')
      continue;
    v[j++] = p[i];
  }

  // Add a null terminator if result string buffer was not filled.
  if (j < maxlen)
    v[j] = '\0';

  return true;
}

boolean Tinkeringtech_CELLSTICK::sendParseReply(const __FlashStringHelper *tosend,
				      const __FlashStringHelper *toreply,
				      uint16_t *v, char divider, uint8_t index) {
  getReply(tosend);

  if (! parseReply(toreply, v, divider, index)) return false;

  readline(); // eat 'OK'

  return true;
}
