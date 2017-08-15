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
bool _got_tag = false;

const byte numChars = 32;
char receivedChars[numChars];

//Mode flags, initiated by hardware interactions
const char rfidflag = 'f';

//Command flags, sent from the iot master (photon or pycom)
const char headingflag = 'h';
const char waitflag = 'w';
const char successflag = 's';
const char errorflag = 'e';
const char printflag = 'p';
const char rgbflag = 'c';
const char retrievalflag = 'g';
const char sleepflag = 'z';
char _mode = 'h';
char _modecache = 'h';

int cueColor[] = {125,125,125};

boolean newData = false;
String cmdcache = "";
long previousMillis = 0;
byte heading_offset_index = 0;
long current_tag = 0;

byte targetled = 0;
long fadecounter = 0;
long fadeinterval = 3500;
bool fadedirection = true;
bool _shouldFlash = false;
int _flashcount = 0;
int _leftheld = 0;
int _rightheld = 0;
byte _feedbackloops = 0;
int _btninterval = 750;
byte brightness = 100;
int base_brtns = brightness;
int brtns_mod = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LEFTBTN, INPUT);
  pinMode(RIGHTBTN, INPUT);
  if(!bno.begin())
  {
    Serial.print("<xfaultbno>");
    while(1);
  }
  bno.setExtCrystalUse(true);
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.println("<xfaultnfc>");
    while (1);
  }
  nfc.setPassiveActivationRetries(0x01);
  nfc.SAMConfig();
  strip.begin();
  strip.setBrightness(brightness);
  strip.show();
}

void loop() {
  unsigned long currentMillis = millis();
  recvWithStartEndMarkers();
  if (newData == true) {
      newData = false;
      String temp(receivedChars);
      applySerialCommand(temp);
  }

  if(_mode == sleepflag){
    strip.setBrightness(0);
    return;
  }

  if(_got_tag){
      int left_state = digitalRead(LEFTBTN);
      int right_state = digitalRead(RIGHTBTN);
      if(left_state == HIGH){
        _rightheld = 0;
        _leftheld++;
        if(_leftheld > _btninterval){
          Serial.print("<fy");
          Serial.print(current_tag, DEC);
          Serial.print(">");
          delay(1000);
          _got_tag = false;
        }
      }
      if(right_state == HIGH){
        _leftheld = 0;
        _rightheld++;
        if(_rightheld > _btninterval){
          Serial.print("<fn");
          Serial.print(current_tag, DEC);
          Serial.print(">");
          delay(1000);
          _got_tag = false;
        }
      }
  }
  
  if(currentMillis - previousMillis > 75){
    previousMillis = currentMillis;
    sensors_event_t event; 
    bno.getEvent(&event);
    targetled = mapheadingtoLED((int)event.orientation.x, heading_offset_index);
    //Adjust brightness based on angle device is held.
    int attitude = (int)event.orientation.y;
    if(attitude > 80 | attitude < -80){
      attitude = 80;
    }
    base_brtns = map(abs(attitude), 0, 80, brightness, 1);
  }

  //here we catch the loop and possibly revert it back to it's normal state-
  //but the normal state can either be an RGB display or a heading display.
  //hence _modecache.
  if(_feedbackloops > 4){
    _mode = _modecache;
    _feedbackloops = 0;
    //if a command came in while we were showing feedback, enact it.
    applySerialCommand(cmdcache);
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
          brtns_mod = map(fadecounter, 0, fadeinterval, 0, 75);
        }
         else{
          brtns_mod = 0;
          _flashcount = 0;
          _shouldFlash = false;
         }
        }
      displayDitheredHeading(brightness, targetled);
      break;
    }
    //Waiting for a call. Slow white flash.
    case 'w':{
      constantPulse(2);
      brtns_mod = map(fadecounter, 0, fadeinterval, 0, 75);
      halfColorWipe(strip.Color(125, 125, 125));
      break;
    }
    //Got the right tag. Fast flash as a reward.
    case 's':{
      constantPulse(4);
      brtns_mod = map(fadecounter, 0, fadeinterval, 0, 75);
      fullColorWipe(strip.Color(cueColor[0], cueColor[1], cueColor[2]));
      break;
    }
    //Polling for a tag. Color goes steady when we get a good scan.
    case 'c':{
      if(!_got_tag){
        constantPulse(2);
        brtns_mod = map(fadecounter, 0, fadeinterval, 0, 75);
        halfColorWipe(strip.Color(cueColor[0], cueColor[1], cueColor[2]));
      }else{
        brtns_mod = 75;
        fullColorWipe(strip.Color(cueColor[0], cueColor[1], cueColor[2]));
      }
      break;
    }
    case 'e':{ //error state
      halfColorWipe(strip.Color(computeChannel(2), 50, 25));
      break;
    }
    default: 
    break;
  }
  int computed_brtns = base_brtns + brtns_mod;
  if(computed_brtns < 10) computed_brtns = 10;
  strip.setBrightness(computed_brtns);
}

void awaitRFIDscan(){
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  uint8_t data[16];
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success) {
    success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 4, 1, keyuniversal);
    if(success){
      success = nfc.mifareclassic_ReadDataBlock(4, data);
      if(success){
        //Artid's are 32-bit integers. Send it upstream to the photon.
        long read_artid;

            read_artid =  (long)data[0] << 24;
            read_artid += (long)data[1] << 16;
            read_artid += (long)data[2] << 8;
            read_artid += (long)data[3];

        _got_tag = true;
        current_tag = read_artid;
      }
    }
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
  
void constantPulse(byte rate){
  if(fadedirection) fadecounter += rate;
  if(!fadedirection) fadecounter -= rate;
  if(fadecounter > fadeinterval) fadedirection = false;
  if(fadecounter < 1) fadedirection = true;
}

//TODO fade every other LED up and down
byte alternatingPulse(uint32_t c, byte rate){
  if(fadedirection) fadecounter += rate;
  if(!fadedirection) fadecounter -= rate;
  if(fadecounter > fadeinterval) fadedirection = false;
  if(fadecounter < 1) fadedirection = true;
  if(fadedirection){
    for (int q=0; q < 3; q++) {
      for (int i=0; i < PIXELS; i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
    }
  }
  return 0;
}

byte rfidscanPulse(byte rate){
  if(fadedirection) fadecounter += rate;
    if(!fadedirection) fadecounter -= rate;
    if(fadecounter > fadeinterval){
      fadedirection = false;
      awaitRFIDscan();
    } 
    if(fadecounter < 1) {
      fadedirection = true;
      awaitRFIDscan();
    }
    return fadecounter;
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
  if(heading > 351) heading = 0;
  byte led = 0;
  int _heading = heading; //355
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

void displayDitheredHeading(byte intensity, byte remnant){
  byte prev = 0;
  byte next = 0;
  byte pindex = PIXELS-1;
  if(remnant < pindex){
    next = remnant + 1;
  }
  if(remnant > 0){
    prev = remnant - 1;
  }
  if(remnant == 0){
    prev = pindex;
  }
  for(byte i=0; i<PIXELS; i++) {
      if(i == remnant){
        strip.setPixelColor(i, strip.Color(intensity, intensity, intensity)); 
        continue; 
      }
      if(i == prev | i == next){
        strip.setPixelColor(i, strip.Color(intensity/3,intensity/2,intensity));
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

void halfColorWipe(uint32_t c){
  for(uint16_t i=0; i<PIXELS; i++) {
      strip.setPixelColor(i, strip.Color(0,0,0));  
  }
  for(uint16_t j=0; j<PIXELS; j=j+2) {
      strip.setPixelColor(j, c);  
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
  if(_mode == rfidflag){
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
      else if(receivedChars[0] == sleepflag){
        _mode = sleepflag;
      }
      fadecounter = 0;
}



