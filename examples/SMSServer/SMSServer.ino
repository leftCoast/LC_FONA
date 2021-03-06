#include <SoftwareSerial.h>

// **************************************************************************************************************
//
// This was written for the Adafruit Feather FONA. A 2G SIM800 chip based cell phone. It uses the Adafruit FONA
// Library and makes a cell phone based text server. this is kind of an example program that just says Hello, 
// Rolls dice, and give back a list of commands if it gets a command it can't parse. Typically this would be
// replced by possibly reading some sort of input device and passing back the information.
// 
// **************************************************************************************************************


#include <lilParser.h>
#include <timeObj.h>
#include "LC_fona.h"

#define FONA_RX 9
#define FONA_TX 8

#define COM_BUFF_BYTES  255
#define ANSWER_BYTES    100
#define PN_BUFF_BYTES   20
#define INT_STR_BYTES   10
#define MAX_DICE        12



// Our delightful set of global variables.

enum commands { noCommand, rollEm, sayHello, tell, help };

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
LC_fona fona = LC_fona(&fonaSS);

bool        FONAOnline;
byte        comBuff[COM_BUFF_BYTES];   // Buffer for all comunication.
byte        pnBuff[PN_BUFF_BYTES];     // Buffer for current phone number.
bool        havePN;                    // Is this a valid and current phone numnber?
char        answer[ANSWER_BYTES];      // Buffer for our reply;
char        intStr[INT_STR_BYTES];     // Buffer to hold a int as a string.
int         SMSIndex = 1;
timeObj     serverTimer(100);
lilParser   comParser;


// Standerd setup() runs once at startup.
void setup() {

   comParser.addCmd(rollEm,"ROLL");
   comParser.addCmd(sayHello,"HELLO");
   comParser.addCmd(sayHello,"HI");
   comParser.addCmd(help,"?");
   randomSeed(A0);                                 // Might as well get a random start.
   havePN = false;                                 // We do not yet have a phone number to text to.
   FONAOnline = fona.begin();                      // Fire up FONA.
   while(!FONAOnline) {                            // If the FONA does NOT fire up.. We sit and blink.
      LED(true);
      delay(100);
      LED(false);
      delay(250);
   }
   
}


// This checks the message queue on the phone chip. If there is a message
// It will grab it, along with its phone number and return a pointer to the
// message text. No message, or and error, will return NULL.
char* checkForMessage(void) {

   int   numSMS;
   int   index;
   char* message;
   
   havePN = false;                                                // We don't have a valid one right now.
   message = NULL;                                                // Or this either.
   numSMS = fona.getNumSMS();                                     // Number SMS(s). This one gives bogus numbers at times. Max is 30.
   if (numSMS > 30) {                                             // If its more than it can hold..
      numSMS = 0;                                                 // Call it a mistake. Bogus readings get zero.
   }                                 
   if (numSMS) {                                                  // If we have unread text messages..
      comBuff[1] = SMSIndex++;                                    // Load the com buffer up with the index of the message to check. Bump up the index.                                   
      if (SMSIndex>30) {                                          // If the index is > 30..
         SMSIndex = 1;                                            // Reset the index to 1.
      }
      getSMSMsg(comBuff);                                         // Attempt to retrieve a message from the phone chip.
      if (comBuff[0]==0) {                                        // This says no errors. So lets decode what we got.
         strcpy(pnBuff,&(comBuff[1]));                            // Byte 0 is error, after that is the phone number. Copy it out.
         index = strlen(pnBuff)+2;                                // BEFORE we hack it up, locate the message part. Add two, error byte & EOS byte.
         message = &(comBuff[index]);                             // Here's the message.
         filterPNStr(pnBuff);                                     // NOW we can clear out the phone number junk.
         havePN = strlen(pnBuff)>=7;                              // Well, it needs at least 7 chars.
      }
   } else {                                                       // Else we have no messages. Reset index to 1.
      SMSIndex = 1;
   }
   if (havePN) {                                                  // If we got a seemingly valid phone number..
      return message;                                             // The result is the address of the message. NULL for none.
   } else {                                                       // Else, no valid phone number..
      return NULL;                                                // Give 'em back a NULL. Who knows what went wrong?
   }
}


// handleMessage takes the incomoing message and tries to parse it into a command. If succsefull,
// it hands off the rest of the message to the appropriate handler to deal with it.
void handleMessage(char* message) {

   int   i;
   int   command;
   
   i = 0;
   while(message[i]!='\0') {
      comParser.addChar(toupper(message[i]));
      i++;
   }
   command = comParser.addChar(EOL);
   switch(command) {
      case noCommand : break;
      case rollEm    : doDiceRoll();   break;
      case sayHello  : doSayHello();   break;
      case help      : doHelp();       break;
      default        : doHelp();       break;
   }
}


// Loop() just runs and runs doing the same thing.
void loop() {

   char* message;
    
   if (serverTimer.ding()) {           // If its time to check messages..
      serverTimer.start();             // Reset our timer.
      message = checkForMessage();     // See if we got a message.
      if (message) {                   // If we have a message..
         LED(true);                    // Turn on the LED so the human can see.
         handleMessage(message);       // Handle the message.
         LED(false);                   // Turn off the LED. We're done.
      }
   }
}
         


// *************************************************
// ******************* handlers ********************
// *************************************************


// User has typed Hi or Hello. Lets send back a greeting.
void doSayHello(void) {

   strcpy(&(comBuff[1]),"Greetings!\nI'm an online dice rolling machine.");
   sendSMSMsg(comBuff);                                // Send the reply stored in the com buffer
}


// User has typed ROLL and possibly a nunber of dice. This is where we deal with that.
void doDiceRoll(void) {

   char* numStr; 
   int   numDice;
   int   total;
   int   roll;

   numDice = 1;                                          // We start with 1..
   if (comParser.numParams()) {                          // If they typed in something past the command.
      numStr = comParser.getParamBuff();                 // We get the first parameter, assume its the number of dice.
      numDice = atoi(numStr);                            // Parse the actual number of dice they call for.
      free(numStr);                                      // Dump the parameter buffer ASAP.
      if (numDice==0) {                                  // If the number of dice is zero..
         numDice = 1;                                    // We'll give them one anyhow.
      }
   }
   if (numDice>MAX_DICE) {                               // If the number of dice is more than the max..
      strcpy(answer,"Too many dice! Max # is ");     		// Set the answer string to an error message.
      strcat(answer,str(MAX_DICE));                      //
      strcat(answer,".");                                //
   } else {                                              // Else, the number of dice is ok..
      strcpy(answer,"Rolling ");                         // Send back header.
      strcat(answer,str(numDice));                       //
      if (numDice==1) {                                  //
         strcat(answer," die.\n");                       //
      } else {
         strcat(answer," dice.\n");
      }
      total = 0;                                         // Zero out our total.
      for (int i=1;i<=numDice;i++) {                     // For each die..
         roll = random(1,7);                             // Get a random value 1..6.
         total = total + roll;                           // Add it to the total.
         strcat(answer,str(roll));                       // Add the value to the answer string.
         if (i!=numDice) {                               // If there are more dice to roll..
            strcat(answer,", ");                         // We add a comma to the answer.
         } else {                                        // Else, we rolled them all..
            strcat(answer,"\nTotal : ");                 // Add "Total :" to the answer string.
            strcat(answer,str(total));                   // Add the actual total to the anser string.
         }
      }
   }
   strcpy(&(comBuff[1]),answer);                         // Copy the assemebled answer to the com buffer. 
   sendSMSMsg(comBuff);                                  // Send the reply stored in the com buffer
}


void doHelp(void) {

   strcpy(&(comBuff[1]),"Msg HELLO and I'll say hello back.\nType ROll and a number and I'll roll dice for you.");
   sendSMSMsg(comBuff);                                // Send the reply stored in the com buffer
}



// *************************************************
// **************** utilty functions ***************
// *************************************************


// If there is a current phone number set, A text string starting at char[1] of buff
// will be sent out to that number.
void sendSMSMsg(byte* buff) {

  if (havePN) {                                    // Sanity, do we even have a phone number?
    if (fona.sendSMS(pnBuff, (char*)&buff[1])) {   // Let the FONA code have a go at sending the message.
      buff[0] = 0;                                 // Success, 0 errors.
    } else {                                       // else FONA didn't like it for some reason.
      buff[0] = 1;                                 // Note the error.
    }
  } else {                                         // We don't have a phone number to text to.
    buff[0] = 2;                                   // Note the error.
  }
}


// The SECOND byte is the index of the message we're after. (I forgot why). When we pack
// our reply, the FIRST byte will be error byte. Then two c-strings. First is sending
// phone number, second is the text message itself. Once a message is read, it's deleted
// from the SIM chip.
void getSMSMsg(byte* buff) {

  uint16_t  numPNBytes;
  uint16_t  numMsgBytes;
  uint16_t  buffLen;
  char*     buffPtr;
  byte      msgIndex;

  msgIndex = buff[1];                                             // What message are we talking about here?
  buffPtr = &(buff[1]);                                           // In the buffer, this is where our reply message starts.
  buffLen = COM_BUFF_BYTES - 2;                                   // How many bytes we have to work wih here? (Subtract 2 for luck.)
  if (fona.getSMSSender(msgIndex, buffPtr, buffLen)) {            // First we read out the phone number..
    numPNBytes = strlen(buffPtr) + 1;                             // They don't tell us how many bytes they used. So we count 'em ourselves.
    buffPtr = buffPtr + numPNBytes;                               // Offset the buffer pointer.
    buffLen = buffLen - numPNBytes;                               // Reset the buffer length.
    if (fona.readSMS(msgIndex, buffPtr, buffLen, &numMsgBytes)) { // If we are successfull..
      fona.deleteSMS(msgIndex);                                   // Delete the message from the SIM.
      buff[0] = 0;                                                // Set no error flag.
      return;                                                     // At this point its success and we're done!
    }
  }
  buff[0] = 1;                                                    // We got here? Note the error.
}


// Drop in a c string and this'll strip out anything
// that's not a "dial-able" character.
// ** WARNING ** This writes to the string in place. So you can't pass a string that was
// allocated at compile time IE char myNum = "1 408 340-0352"
void filterPNStr(char* str) {

   int numChars;
   int index;
   
   if (str) {                          // Sanity, they could pass in a NULL. They do that now and then.
      numChars = strlen(str);          // Ok have something. Lets count 'em.
      index = 0;                       // We'll use this to index mRawPN.
      for(int i=0;i<=numChars;i++) {   // We'll loop though including the EOL.
         switch(str[i]) {              // There may be better ways of doing this,
            case '0'  :                //  but this makes it realaly obvious what we're doing.
            case '1'  :
            case '2'  :
            case '3'  :
            case '4'  :
            case '5'  :
            case '6'  :
            case '7'  :
            case '8'  :
            case '9'  :
            case '#'  :
            case '*'  :
            case '\0' : str[index++] = str[i]; break;  // Stuff in the "filtered" char.
         }
      }
   }
}


// Drop in an integer and out pops a pointer to the string version of it.
char* str(int value) { 

   snprintf(intStr,10,"%d",value);
   return intStr;
}


// Lets make turing the LED on and off easy.
void LED(bool onOff) { digitalWrite(13,onOff); }
