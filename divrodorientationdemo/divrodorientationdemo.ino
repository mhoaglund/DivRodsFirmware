#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_NeoPixel.h>
#define DATAPIN 2
 
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, DATAPIN, NEO_GRB + NEO_KHZ800);
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
 
void setup() {
  Serial.begin(9600);
    /* Initialise the sensor */
  if(!mag.begin())
  {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }
  strip.begin();
  strip.setBrightness(30); //adjust brightness here
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // Some example procedures showing how to display to the pixels:
    /* Get a new sensor event */ 
  sensors_event_t event; 
  mag.getEvent(&event);


  //with the magnetometer's x axis pointed up
  float heading = atan2(event.magnetic.y, event.magnetic.x);
  
  // Once you have your heading, you must then add your 'Declination Angle', which is the 'Error' of the magnetic field in your location.
  // Find yours here: http://www.magnetic-declination.com/
  // Minneapolis is 0.19
  float declinationAngle = 0.19;
  heading += declinationAngle;
  
  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;
   
  // Convert radians to degrees for readability.
  float headingDegrees = heading * 180/M_PI; 
  int headingint = (int)headingDegrees;
  int targetled = mapheadingtoLED(headingint);
  strip.setPixelColor(targetled, strip.Color(255, 0, 255));
  instantColorWipe(strip.Color(0, 125, 155), targetled);
  Serial.print("Heading (degrees): "); Serial.println(headingDegrees);
  
  delay(50);
}
 
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
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
 
void rainbow(uint8_t wait) {
  uint16_t i, j;
 
  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

uint32_t mapheadingtoLED(int heading){
  uint32_t led = 0;
  if(heading >= 0 & heading < 45) led = 0;
  if(heading >= 45 & heading < 90) led = 1;
  if(heading >= 90 & heading < 135) led = 2;
  if(heading >= 135 & heading < 180) led = 3;
  if(heading >= 180 & heading < 225) led = 4;
  if(heading >= 225 & heading < 270) led = 5;
  if(heading >= 270 & heading < 315) led = 6;
  if(heading >= 315 & heading < 360) led = 7;
  return led;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
 
