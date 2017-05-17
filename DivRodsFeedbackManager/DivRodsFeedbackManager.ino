#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>

//DivRods FeedbackManager Firmware
//Developed by maxwell.hoaglund@gmail.com

//This firmware is meant to run on an arduino mini, acting as an asynchronous feedback
//manager to compliment the often-busy Particle Photon which manages interaction with
//our cloud API.

//The device this firmware runs on will be connected to our Photon as a slave via SPI.
//When the Photon gets new information about user's location, it will notify this device
//so heading guidance can be kept current.

//Peripherals managed by this device include a BNO055 AOS, a neopixel ring, and two haptic feedback motors.

//Target is Arduino 328.
#define RING_DATA_PIN 6

Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, RING_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_BNO055 bno = Adafruit_BNO055(55);

const byte numChars = 32;
char receivedChars[numChars];

const char headingflag = 'h';
const char waitflag = 'w';
const char successflag = 's';
char _mode = 'h';

boolean newData = false;

int CURRENT_HEADING = 0;
long previousMillis = 0;
const int heading_interval = 75;
byte heading_offset_index = 0;

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
  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  delay(250);
    
  bno.setExtCrystalUse(true);
  strip.begin();
  strip.setBrightness(150); //adjust brightness here
  strip.show(); // Initialize all pixels to 'off'
}

int targetled = 0;
long fadecounter = 0;
long fadeinterval = 1500;
bool fadedirection = true;
void loop() {
  unsigned long currentMillis = millis();
  recvWithStartEndMarkers();
  parseSerialPacket();
  if(currentMillis - previousMillis > heading_interval){
    previousMillis = currentMillis;
    sensors_event_t event; 
    bno.getEvent(&event);
    int headingint = (int)event.orientation.x;
    targetled = mapheadingtoLED(headingint, LED_MAP[heading_offset_index]);
    int attitude = (int)event.orientation.y;
    if(attitude > 45){
      strip.setBrightness(15);
    }else{
      strip.setBrightness(150);
    }
  }
  
  switch (_mode) {
    case 'h':{
      strip.setPixelColor(targetled, strip.Color(255, 0, 255));
      instantColorWipe(strip.Color(0, 125, 155), targetled);
      break;
    }
    case 'w':{
      if(fadedirection) fadecounter++;
      if(!fadedirection) fadecounter--;
      if(fadecounter > fadeinterval) fadedirection = false;
      if(fadecounter < 1) fadedirection = true;
      int rchan = map(fadecounter, 0, fadeinterval, 2, 145);
      int bchan = map(fadecounter, 0, fadeinterval, 255, 25);
      fullColorWipe(strip.Color(rchan, bchan, 125));
      break;
    }
    case 's':{
      if(fadedirection) fadecounter += 5;
      if(!fadedirection) fadecounter -= 5;
      if(fadecounter > fadeinterval) fadedirection = false;
      if(fadecounter < 1) fadedirection = true;
      int gchan = map(fadecounter, 0, fadeinterval, 25, 255);
      int bchan = map(fadecounter, 0, fadeinterval, 255, 25);
      fullColorWipe(strip.Color(0, gchan, bchan));
      break;
    }
    default: 
    break;
  }
}

uint32_t mapheadingtoLED(int heading, byte indices[]){
  uint32_t led = 0;
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

uint32_t roundHeading(int heading){
  uint32_t led = 0;
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
void instantColorWipe(uint32_t c, int remnant){
  for(uint16_t i=0; i<strip.numPixels(); i++) {
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

void parseSerialPacket() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChars);
        newData = false;
        String temp(receivedChars);
        if(receivedChars[0] == headingflag){
          _mode = 'h';
          Serial.println(temp);
          temp.remove(0,1);
          heading_offset_index = roundHeading(temp.toInt());
          Serial.print("Processed heading: ");
          Serial.println(heading_offset_index);
        }
        else if(receivedChars[0] == waitflag){
          _mode = 'w';
        }
        else if(receivedChars[0] == successflag){
          _mode = 's';
        }
    }
}

