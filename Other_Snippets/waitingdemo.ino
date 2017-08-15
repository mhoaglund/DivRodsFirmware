
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
  constantPulse(2); 
  brtns_mod = map(fadecounter, 0, fadeinterval, 0, 75);
  halfColorWipe(strip.Color(125, 125, 125));

  int computed_brtns = base_brtns + brtns_mod;
  if(computed_brtns < 10) computed_brtns = 10;
  strip.setBrightness(computed_brtns);
}

  void constantPulse(byte rate){
  if(fadedirection) fadecounter += rate;
  if(!fadedirection) fadecounter -= rate;
  if(fadecounter > fadeinterval) fadedirection = false;
  if(fadecounter < 1) fadedirection = true;
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