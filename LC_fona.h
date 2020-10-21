#ifndef LC_fona_h
#define LC_fona_h

#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"

// Defaults?
#define FONA_RST 4
#define FONA_RI  7


class LC_fona : public Adafruit_FONA {

	public:
				LC_fona(SoftwareSerial* fonaSS,bool callerID=false);
	virtual	~LC_fona(void);

	bool		begin(void);
	bool		resetFONA(void);
	bool		setParam(FONAFlashStringPtr send, int32_t param);
	bool		checkForCallerID(char* IDBuff, byte numBytes);

	bool						mInit;
	bool						mUseCallerID;
	SoftwareSerial*		mFonaSS;
	FONAFlashStringPtr	ok_reply;
};


#endif