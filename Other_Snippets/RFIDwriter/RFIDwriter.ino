#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

uint32_t artidlong = 111619;
uint8_t artid[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t myblock = 4;

boolean newData = false;
boolean shouldWrite = false;
const byte numChars = 32;
char receivedChars[numChars];

const char idflag = 'a';

void setup() {
  set_byte_array(artidlong);
  Serial.begin(9600);
  Serial.println("Looking for PN532...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
}

void set_byte_array(uint32_t input){
  // convert from an unsigned long int to a 4-byte array
  artid[0] = (int)((input >> 24) & 0xFF);
  artid[1] = (int)((input >> 16) & 0xFF);
  artid[2] = (int)((input >> 8) & 0XFF);
  artid[3] = (int)((input & 0XFF));  
}

void loop(void) {
  recvWithStartEndMarkers();
  if (newData == true) {
      newData = false;
      String temp(receivedChars);
      applySerialCommand(temp);
  }
  
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  bool authenticated = false;               // Flag to indicate if the sector is authenticated

  // Use the default NDEF keys (these would have have set by mifareclassic_formatndef.pde!)
  uint8_t keya[6] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5 };
  uint8_t keyb[6] = { 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 };
  uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  // Wait for user input before proceeding
  //while (!Serial.available());
  // a key was pressed1
  //while (Serial.available()) Serial.read();
    
  // Wait for an ISO14443A type card (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  if(!shouldWrite) return;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) 
  {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (uidLength != 4)
    {
      Serial.println("Ooops ... this doesn't seem to be a Mifare Classic card!"); 
      return;
    }
    
    // We probably have a Mifare Classic card ... 
    Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

    // Check if this is an NDEF card (using first block of sector 1 from mifareclassic_formatndef.pde)
    // Must authenticate on the first key using 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7
    success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 4, 0, keyuniversal);
    
    if (!success)
    {
      Serial.println("Unable to authenticate block 4 ... Card may be defective");
      return;
    }

    Serial.println("Authentication succeeded!");
    Serial.println("Updating sector 4 with art id");
    
    // URI is within size limits ... write it to the card and report success/failure
    //success = nfc.mifareclassic_WriteNDEFURI(target_sector, ndefprefix, artid);
    success = nfc.mifareclassic_WriteDataBlock(myblock, artid);
    if (success)
    {
      Serial.println("Art ID Record written to sector 4");
      Serial.println("");      
    }
    else
    {
      Serial.println("NDEF Record creation failed! :(");
    }
  }
  
  // Wait a bit before trying again
  Serial.println("\n\nDone!");
  shouldWrite = false;
  //delay(500);
//  Serial.flush();
//  while(Serial.available()) Serial.read();
}


void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void applySerialCommand(String serialcommand){
  if(serialcommand[0] == idflag){
    serialcommand.remove(0,1);
    char charBuf[50];
    serialcommand.toCharArray(charBuf, 50);
    long lint = atol(charBuf);
    artidlong = lint;
    set_byte_array(artidlong);
    Serial.print("Updated Art ID to: ");
    Serial.println(lint, DEC);
  }
  if(serialcommand[0] == 'w'){
    shouldWrite = true;
  }
}

