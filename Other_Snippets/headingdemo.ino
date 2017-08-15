
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define RING_DATA_PIN 6
#define PIXELS 12

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXELS, RING_DATA_PIN, NEO_GRB + NEO_KHZ800);

int cueColor[] = {125,125,125};

long previousMillis = 0;
byte heading_offset_index = 0;

byte targetled = 0;
long fadecounter = 0;
long fadeinterval = 3500;
bool fadedirection = true;

byte min_heading = 45;
byte max_heading = 190;
byte _heading = 45;
bool headingdirection = true;

byte brightness = 100;
int base_brtns = brightness;
int brtns_mod = 0;

void setup() {
  strip.begin();
  strip.setBrightness(brightness);
  strip.show();
}

void loop() {
  unsigned long currentMillis = millis();  
  if(currentMillis - previousMillis > 200){
    previousMillis = currentMillis;
    updateFakeHeading();
    targetled = mapheadingtoLED(_heading, heading_offset_index);
    //Adjust brightness based on angle device is held.
    int attitude = (int)event.orientation.y;
    if(attitude > 80 | attitude < -80){
      attitude = 80;
    }
    base_brtns = map(abs(attitude), 0, 80, brightness, 1);
  }
  displayDitheredHeading(brightness, targetled);
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

void updateFakeHeading(){
    if(headingdirection){
        _heading++;
        if(_heading > max_heading){
            headingdirection = false;
            return;
        }
    }
    else{
        _heading--
        if(_heading < min_heading) {
            headingdirection = true;
            return;
        }
    }
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