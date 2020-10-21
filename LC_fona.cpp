#include "LC_fona.h"


LC_fona::LC_fona(SoftwareSerial* fonaSS,bool useCallerID)
  : Adafruit_FONA(FONA_RST) {
  
  mFonaSS		= fonaSS;
  mUseCallerID	= useCallerID;
  ok_reply		= F("OK");
  mInit			= false;
}


LC_fona::~LC_fona(void) {  }


// Called by setup. This resets the FONA hardware and then sets up the correct
// configuration for the phone chip.
bool LC_fona::begin(void) {

	pinMode(0, INPUT);					// Adafruit says to do this. Otherwise it may read noise.
   pinMode(FONA_RST, OUTPUT);			// Used for resetting the FONA.
	digitalWrite(FONA_RST, HIGH);
	mInit = true;
	mFonaSS->begin(4800);				// For talking to the FONA.
	return resetFONA();
}


// If for some reason, on the fly, you need to reset the FONA? This is the call.
bool LC_fona::resetFONA(void) {

	bool	online;
	
	online = false;
	digitalWrite(FONA_RST, HIGH);
	delay(100);
	digitalWrite(FONA_RST, LOW);
	delay(10);
	digitalWrite(FONA_RST, HIGH);
	delay(100);
	online = Adafruit_FONA::begin(*mFonaSS);	// Able to fire up the FONA.
	if (online) {
		if (mUseCallerID) {
			setParam(F("AT+CLIP="), 1);				// Set caller ID (0 = off).
		} else {
			setParam(F("AT+CLIP="), 0);
		}
		//fona.enableNetworkTimeSync(true);			// See if it works..
	}
	return(online);
}


// The one we were missing. Setting a param easily. I forget why..
bool LC_fona::setParam(FONAFlashStringPtr send, int32_t param) {

  return sendCheckReply(send, param, ok_reply);
}


bool  LC_fona::checkForCallerID(char* IDBuff, byte numBytes) {

  if (mySerial->available()) {                                          // Is something is there we didn't ask for..
    if (readline(25,true) > 15) {                                      // See if we can grab the first 20 or so chars.
      return parseReplyQuoted(F("+CLIP: "), IDBuff, numBytes, ',', 0);  // Run it through the Adafruit parser thing to see if its a caller ID.
    }
  }
  return false;
}

