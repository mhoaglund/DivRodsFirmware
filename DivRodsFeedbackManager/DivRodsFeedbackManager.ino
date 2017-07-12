#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#include <SPI.h>
#include <Adafruit_PN532.h>
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

//DivRods FeedbackManager Firmware
//Developed by maxwell.hoaglund@gmail.com

//This firmware is meant to run on an arduino mini, acting as an asynchronous feedback
//manager to compliment the often-busy Particle Photon which manages interaction with
//our cloud API.

//The device this firmware runs on will be connected to our Photon as a slave via SPI.
//When the Photon gets new information about user's location, it will notify this device
//so heading guidance can be kept current.

//Peripherals managed by this device include a BNO055 AOS, a neopixel ring, two buttons, and two haptic feedback motors.

#define LEFTBTN 5
#define RIGHTBTN 4
//Target is Arduino 328.
#define RING_DATA_PIN 6

Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, RING_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_BNO055 bno = Adafruit_BNO055(55);
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

#define OLED_RESET 10
//Adafruit_SSD1306 display(OLED_RESET); //30% of memory right here!

const byte numChars = 32;
char receivedChars[numChars];

//Mode flags, initiated by hardware interactions
const char rfidflag = 'f';
const char rightflag = 'r';
const char leftflag = 'l';

//Command flags, sent from the iot master (photon or pycom)
const char headingflag = 'h';
const char waitflag = 'w';
const char successflag = 's';
const char errorflag = 'e';
const char printflag = 'p';
const char rgbflag = 'c';
char _mode = 'h';

int cueColor[] = {125,125,125};

boolean newData = false;
String cmdcache = "";
long previousMillis = 0;
byte heading_offset_index = 0;

//TODO redo this so it isn't a global.
 byte LED_MAP[][12] = {
  {0,1,2,3,4,5,6,7,8,9,10,11},
  {11,0,1,2,3,4,5,6,7,8,9,10},
  {10,11,0,1,2,3,4,5,6,7,8,9},
  {9,10,11,0,1,2,3,4,5,6,7,8},
  {8,9,10,11,0,1,2,3,4,5,6,7},
  {7,8,9,10,11,0,1,2,3,4,5,6},
  {6,7,8,9,10,11,0,1,2,3,4,5},
  {5,6,7,8,9,10,11,0,1,2,3,4},
  {4,5,6,7,8,9,10,11,0,1,2,3},
  {3,4,5,6,7,8,9,10,11,0,1,2},
  {2,3,4,5,6,7,8,9,10,11,0,1},
  {1,2,3,4,5,6,7,8,9,10,11,0}
 };

void setup() {
  Serial.begin(9600);
  pinMode(LEFTBTN, INPUT);
  pinMode(RIGHTBTN, INPUT);
  if(!bno.begin())
  {
    Serial.print("<xfault>");
    while(1);
  }
  nfc.begin();
  nfc.setPassiveActivationRetries(0x01);
  nfc.SAMConfig();

  bno.setExtCrystalUse(true);
  strip.begin();
  strip.setBrightness(150); //adjust brightness here
  strip.show(); // Initialize all pixels to 'off'
}

byte targetled = 0;
long fadecounter = 0;
long fadeinterval = 1500;
bool fadedirection = true;
bool _shouldFlash = false;
int _flashcount = 0;
int _leftheld = 0;
int _rightheld = 0;
byte _maxfeedbackloops = 4;
byte _feedbackloops = 0;
int _btninterval = 750;
bool _hasPressed = false;
void loop() {
  unsigned long currentMillis = millis();
  recvWithStartEndMarkers();
  if (newData == true) {
      newData = false;
      String temp(receivedChars);
      applySerialCommand(temp);
  }

  if(_hasPressed == false){
    int left_state = digitalRead(LEFTBTN);
    int right_state = digitalRead(RIGHTBTN);
    if(left_state == LOW){
      _rightheld = 0;
      _leftheld++;
      if(_leftheld > _btninterval){
        _mode = leftflag;
        _feedbackloops = 0;
        _leftheld = 0;
        _hasPressed = true;
      }
    }
    if(right_state == LOW){
      _leftheld = 0;
      _rightheld++;
      if(_rightheld > _btninterval){
        _mode = rightflag;
        _feedbackloops = 0;
        _rightheld = 0;
        _hasPressed = true;
      }
    } 
  }
  
  if(currentMillis - previousMillis > 75){
    previousMillis = currentMillis;
    sensors_event_t event; 
    bno.getEvent(&event);
    int headingint = (int)event.orientation.x;
    targetled = mapheadingtoLED(headingint, LED_MAP[heading_offset_index]);
    int attitude = (int)event.orientation.y;
    if(attitude > 35){
      strip.setBrightness(15);
    }else{
      strip.setBrightness(125);
    }
  }

  if(_feedbackloops > _maxfeedbackloops){
    _mode = headingflag;
    _feedbackloops = 0;
    //if a command came in while we were showing feedback, enact it.
    applySerialCommand(cmdcache);
    _hasPressed = false;
  }
  
  switch (_mode) {
    case 'h':{
      if(_shouldFlash){
        if(_flashcount < 4){
          if(fadedirection) fadecounter += 5;
          if(!fadedirection) fadecounter -= 5;
          if(fadecounter > fadeinterval) fadedirection = false;
          if(fadecounter < 1){
            fadedirection = true;
            _flashcount++;
          }
          byte brightness = map(fadecounter, 0, fadeinterval, 150, 250);
          strip.setBrightness(brightness);
        }
         else{
          strip.setBrightness(150);
          _flashcount = 0;
          _shouldFlash = false;
         }
        }
      strip.setPixelColor(targetled, strip.Color(255, 0, 255));
      instantColorWipe(strip.Color(0, 125, 155), targetled);
      break;
    }
    case 'w':{
      if(fadedirection) fadecounter++;
      if(!fadedirection) fadecounter--;
      if(fadecounter > fadeinterval) fadedirection = false;
      if(fadecounter < 1) fadedirection = true;
      byte rchan = map(fadecounter, 0, fadeinterval, 2, 145);
      byte bchan = map(fadecounter, 0, fadeinterval, 255, 25);
      fullColorWipe(strip.Color(rchan, bchan, 125));
      break;
    }
    case 's':{
      if(fadedirection) fadecounter += 5;
      if(!fadedirection) fadecounter -= 5;
      if(fadecounter > fadeinterval) fadedirection = false;
      if(fadecounter < 1) fadedirection = true;
      byte gchan = map(fadecounter, 0, fadeinterval, 25, 255);
      byte bchan = map(fadecounter, 0, fadeinterval, 255, 25);
      fullColorWipe(strip.Color(0, gchan, bchan));
      break;
    }
        case 'r':{
          //user registering approval. go blue and scan for RFID
          fullColorWipe(strip.Color(15, 25, computeChannel(2)));
          awaitRFIDscan();
          break;
        }
        case 'l':{
          //user registering disapproval. go yellow and scan for RFID
          byte channel = computeChannel(2);
          fullColorWipe(strip.Color(channel, channel, 0));
          awaitRFIDscan();
          break;
        }
        case 'c':{
          //displaying a cue color to direct user to scan a matching tag.
          if(_shouldFlash){
            if(_flashcount < 4){
              if(fadedirection) fadecounter += 1;
              if(!fadedirection) fadecounter -= 1;
              if(fadecounter > fadeinterval) fadedirection = false;
              if(fadecounter < 1){
                fadedirection = true;
                _flashcount++;
              }
              int brightness = map(fadecounter, 0, fadeinterval, 150, 250);
              strip.setBrightness(brightness);
            }
            else{
              strip.setBrightness(150);
              _flashcount = 0;
              _shouldFlash = false;
            }
            }
          fullColorWipe(strip.Color(cueColor[0], cueColor[0], cueColor[0]));
          break;
        }
    case 'e':{ //error state
      fullColorWipe(strip.Color(computeChannel(2), 50, 25));
      break;
    }
    default: 
    break;
  }
}

void awaitRFIDscan(){
  if(fadedirection) fadecounter += increment;
  if(!fadedirection) fadecounter -= increment;
  if(fadecounter > fadeinterval){
    fadedirection = false;
    _feedbackloops++;
  }
  if(fadecounter < 1) fadedirection = true;
  
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    auth = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 4, 1, keyuniversal);
    if(auth){
      blockdata = nfc.mifareclassic_ReadDataBlock(4, data);
      if(blockdata){
        Serial.print("<f");
        //Artid's are 32-bit integers. Send it upstream to the photon.
        long read_artid;

            read_artid =  (long)data[0] << 24;
            read_artid += (long)data[1] << 16;
            read_artid += (long)data[2] << 8;
            read_artid += (long)data[3];

        Serial.print(read_artid, DEC);
        Serial.print(">");
      }
    }
  delay(100);
  }
}

byte computeChannel(byte increment){
  //user registering approval. go blue.
  if(fadedirection) fadecounter += increment;
  if(!fadedirection) fadecounter -= increment;
  if(fadecounter > fadeinterval){
    fadedirection = false;
    _feedbackloops++;
  }
  if(fadecounter < 1) fadedirection = true;
  byte chan = map(fadecounter, 0, fadeinterval, 25, 255);
  return chan;
}

byte mapheadingtoLED(int heading, byte indices[]){
  byte led = 0;
  if(heading >= 0 & heading < 30) led = indices[11];
  if(heading >= 30 & heading < 60) led = indices[10];
  if(heading >= 60 & heading < 90) led = indices[9];
  if(heading >= 90 & heading < 120) led = indices[8];
  if(heading >= 120 & heading < 150) led = indices[7];
  if(heading >= 150 & heading < 180) led = indices[6];
  if(heading >= 180 & heading < 210) led = indices[5];
  if(heading >= 210 & heading < 240) led = indices[4];
  if(heading >= 240 & heading < 270) led = indices[3];
  if(heading >= 270 & heading < 300) led = indices[2];
  if(heading >= 300 & heading < 330) led = indices[1];
  if(heading >= 330 & heading < 360) led = indices[0];
  return led;
}

byte roundHeading(int heading){
  byte led = 0;
  if(heading >= 0 & heading < 30) led = 11;
  if(heading >= 30 & heading < 60) led = 10;
  if(heading >= 60 & heading < 90) led = 9;
  if(heading >= 90 & heading < 120) led = 8;
  if(heading >= 120 & heading < 150) led = 7;
  if(heading >= 150 & heading < 180) led = 6;
  if(heading >= 180 & heading < 210) led = 5;
  if(heading >= 210 & heading < 240) led = 4;
  if(heading >= 240 & heading < 270) led = 3;
  if(heading >= 270 & heading < 300) led = 2;
  if(heading >= 300 & heading < 330) led = 1;
  if(heading >= 330 & heading < 360) led = 0;
  return led;
}

//Set color of all but passed in pixel
void instantColorWipe(uint32_t c, byte remnant){
  for(byte i=0; i<strip.numPixels(); i++) {
      if(i != remnant){
        strip.setPixelColor(i, c);  
      }
  }
  strip.show();
}

//Set color of all but passed in pixel
void fullColorWipe(uint32_t c){
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);  
  }
  strip.show();
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
  if(_mode == rfidflag | _mode == leftflag | _mode == rightflag){
        //cache a command that came in during button interaction
        cmdcache = serialcommand;
        return;
      }
      if(serialcommand[0] == headingflag){
        _mode = headingflag;
        _shouldFlash = true;
        serialcommand.remove(0,1);
        heading_offset_index = roundHeading(serialcommand.toInt());
      }
      else if(receivedChars[0] == rgbflag){
        _mode = rgbflag;
        char* rgbvals = strtok(receivedChars, ".");
        byte i = 0;
        while (rgbvals != 0){
          int color = atoi(rgbvals);
            cueColor[i] = color;
            i++;
          rgbvals = strtok(0, "&");
        }
        //TODO: send rbg vals from photon and figure out how to parse
      }
      else if(receivedChars[0] == waitflag){
        _mode = waitflag;
      }
      else if(receivedChars[0] == successflag){
        _mode = successflag;
      }
      else if(receivedChars[0] == errorflag){
        _mode = errorflag;
      }
      else if(receivedChars[0] == printflag){
//        display.clearDisplay();
//        display.setCursor(0,0);
//        serialcommand.remove(0,1);
//        display.println(serialcommand);
//        display.display();
      }
      fadecounter = 0;
  
}

