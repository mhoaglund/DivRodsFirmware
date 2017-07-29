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

#define LEFTBTN 9
#define RIGHTBTN 8
#define RING_DATA_PIN 6
#define PIXELS 16

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS, RING_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_BNO055 bno = Adafruit_BNO055(55);
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
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
const char retrievalflag = 'g';
char _mode = 'h';
char _modecache = 'h';
char _pref = 'n';

int cueColor[] = {125,125,125};

boolean newData = false;
String cmdcache = "";
long previousMillis = 0;
byte heading_offset_index = 0;
long _most_recent_tag = 0;
char _most_recent_pref = 'y';

byte brightness = 100;

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
  nfc.setPassiveActivationRetries(0x08);
  nfc.SAMConfig();

  bno.setExtCrystalUse(true);
  strip.begin();
  strip.setBrightness(brightness); //adjust brightness here
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
    targetled = mapheadingtoLED(headingint, heading_offset_index);
    int attitude = (int)event.orientation.y;
    if(attitude > 35){
      strip.setBrightness(15);
    }else{
      strip.setBrightness(brightness);
    }
  }

  //here we catch the loop and possibly revert it back to it's normal state-
  //but the normal state can either be an RGB display or a heading display.
  //hence _modecache.
  if(_feedbackloops > _maxfeedbackloops){
    _mode = _modecache;
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
          byte brightness = map(fadecounter, 0, fadeinterval, brightness, 175);
          strip.setBrightness(brightness);
        }
         else{
          strip.setBrightness(150);
          _flashcount = 0;
          _shouldFlash = false;
         }
        }
      //strip.setPixelColor(targetled, strip.Color(255, 0, 255));
      //instantColorWipe(strip.Color(0, 125, 155), targetled);
      displayDitheredHeading(200, targetled);
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
          _pref = 'y';
          fullColorWipe(strip.Color(15, 25, buttonFeedbackLoop(2)));
          break;
        }
        case 'l':{
          //user registering disapproval. go yellow and scan for RFID
          _pref = 'n';
          byte channel = buttonFeedbackLoop(2);
          fullColorWipe(strip.Color(channel, channel, 0));
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
          fullColorWipe(strip.Color(cueColor[0], cueColor[1], cueColor[2]));
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
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  uint8_t data[16];
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  _feedbackloops++;
 if (success) {
   success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 4, 1, keyuniversal);
   if(success){
     success = nfc.mifareclassic_ReadDataBlock(4, data);
     if(success){
       Serial.print("<f");
       Serial.print(_pref);
       //Artid's are 32-bit integers. Send it upstream to the photon.
       long read_artid;

           read_artid =  (long)data[0] << 24;
           read_artid += (long)data[1] << 16;
           read_artid += (long)data[2] << 8;
           read_artid += (long)data[3];

       Serial.print(read_artid, DEC);
       Serial.print(">");
       _most_recent_tag = read_artid;
       _most_recent_pref = _pref;
       _feedbackloops = _maxfeedbackloops + 1;
     }
   }
 delay(100);
 }
}

byte computeChannel(byte increment){
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

byte buttonFeedbackLoop(byte increment){
  if(fadedirection) fadecounter += increment;
  if(!fadedirection) fadecounter -= increment;
  if(fadecounter > fadeinterval){
    fadedirection = false;
    awaitRFIDscan();
  }
  if(fadecounter < 1) fadedirection = true;
  byte chan = map(fadecounter, 0, fadeinterval, 25, 255);
  return chan;
}

byte roundHeading(int heading){
  int _heading = 360 - heading;
  byte _slice = 360 / PIXELS;
  byte index = _heading / _slice;
  return index;
}

byte mapheadingtoLED(int heading, byte offset){
  byte led = 0;
  int _heading = 360 - heading;
  byte _slice = 360 / PIXELS;
  byte index = _heading / _slice;
  byte map[PIXELS];
  byte _set = PIXELS - offset;
  byte _start = (_set < PIXELS) ? _set : 0;
  for(byte i = 0; i<PIXELS; i++){
    map[i] = _start;
    _start++;
    if(_start > (PIXELS -1)){
      _start = 0;
    }
  } 
  led = map[index];
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

void displayDitheredHeading(byte intensity, byte remnant){
  byte prev = 0;
  byte next = 0;
  if(remnant < PIXELS){
    next = remnant + 1;
  }
  if(remnant > 0){
    prev = remnant - 1;
  }
  for(byte i=0; i<PIXELS; i++) {
      if(i == remnant){
        strip.setPixelColor(i, strip.Color(intensity, intensity, intensity)); 
        continue; 
      }
      if(i == prev | i == next){
        strip.setPixelColor(i, strip.Color(intensity/2,intensity/2,intensity/2));
        continue;
      }
      strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();
}

//Set color of all but passed in pixel
void fullColorWipe(uint32_t c){
  for(uint16_t i=0; i<PIXELS; i++) {
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
                receivedChars[ndx] = '\0';
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
        _modecache = headingflag;
        _shouldFlash = true;
        serialcommand.remove(0,1);
        heading_offset_index = roundHeading(serialcommand.toInt());
      }
      else if(receivedChars[0] == rgbflag){
        _mode = rgbflag;
        _modecache = rgbflag;
        _shouldFlash = true;
        char* rgbvals = strtok(receivedChars, ".");
        byte i = 0;
        //packet looks like this: <c.125.125.125>
        while (rgbvals != 0){
            int color = atoi(rgbvals);
            if(color > 0){
                cueColor[i] = color;
                i++;                
            }
          rgbvals = strtok(0, ".");
        }
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
      else if(receivedChars[0] == retrievalflag){
        Serial.print("<f");
        Serial.print(_most_recent_pref);
        Serial.print(_most_recent_tag);
        Serial.print(">");
      }
      fadecounter = 0;
}