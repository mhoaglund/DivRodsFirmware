#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t data[16];

void setup(void) {
  Serial.begin(9600);
  nfc.begin();
  Serial.println("Starting...");
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.println("No NFC module...");
    while (1); // halt
  }
  nfc.setPassiveActivationRetries(0x05);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
}

void loop(void) {
  boolean success;
  boolean auth;
  boolean blockdata;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    auth = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 4, 1, keyuniversal);
    if(auth){
      blockdata = nfc.mifareclassic_ReadDataBlock(4, data);
      if(blockdata){
        Serial.print("<f");
        long read_artid;

            read_artid =  (long)data[0] << 24;
            read_artid += (long)data[1] << 16;
            read_artid += (long)data[2] << 8;
            read_artid += (long)data[3];

        Serial.print(read_artid, DEC);
        Serial.println(">");
      }
    }
  delay(1000);
  }
}
