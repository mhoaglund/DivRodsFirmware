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

//Target is Arduino 328. Initial implementation uses an 8mhz one.
#define RING_DATA_PIN 6

Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, RING_DATA_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_BNO055 bno = Adafruit_BNO055(55);

char buf[100];
volatile byte pos;
volatile bool process_it;

int CURRENT_HEADING = 0;
long previousMillis = 0;
const int heading_interval = 75;
byte heading_offset_index = 0;
byte LED_MAP[][8] = {
    {0,1,2,3,4,5,6,7},
    {7,0,1,2,3,4,5,6},
    {6,7,0,1,2,3,4,5},
    {5,6,7,0,1,2,3,4},
    {4,5,6,7,0,1,2,3},
    {3,4,5,6,7,0,1,2},
    {2,3,4,5,6,7,0,1},
    {1,2,3,4,5,6,7,0}
  };

void setup() {
  Serial.begin(9600);
  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  delay(1000);
    
  bno.setExtCrystalUse(true);
  strip.begin();
  strip.setBrightness(30); //adjust brightness here
  strip.show(); // Initialize all pixels to 'off'
  
  SPCR |= bit (SPE); //somehow this turns on SPI in slave mode. TODO understand
  pinMode(MISO, OUTPUT);
  pos = 0;
  process_it = false;
  SPI.attachInterrupt();
}

// SPI interrupt routine. Asynchronously add a received byte to the pile.
ISR (SPI_STC_vect)
{
byte c = SPDR;  // grab byte from SPI Data Register
  if (pos < sizeof buf){
    buf [pos++] = c;
    if (c == '\n') process_it = true;
   } 
}  // end of interrupt routine SPI_STC_vect
int RXbuf = 0;
void loop() {
  unsigned long currentMillis = millis();
  if (Serial.available() > 0) {
      // read the incoming byte:
      RXbuf = Serial.parseInt();
      if(RXbuf < 8 & RXbuf > 0){
        heading_offset_index = RXbuf;
      }
      // say what you got:
      Serial.print("I received: ");
      Serial.println(RXbuf);
  }
  if (process_it){
      //Should be getting a command byte and a data byte.
      buf [pos] = 0;
      Serial.println (buf);
      if(buf[0]==0) heading_offset_index = buf[1]; //something like that!
      pos = 0;
      process_it = false;
  }
  if(currentMillis - previousMillis > heading_interval){
    previousMillis = currentMillis;
    sensors_event_t event; 
    bno.getEvent(&event);

    int headingint = (int)event.orientation.x;
    int targetled = mapheadingtoLED(headingint, LED_MAP[heading_offset_index]);
    strip.setPixelColor(targetled, strip.Color(255, 0, 255));
    instantColorWipe(strip.Color(0, 125, 155), targetled);
  }
}

uint32_t mapheadingtoLED(int heading, byte indices[]){
  uint32_t led = 0;
  if(heading >= 0 & heading < 45) led = indices[7];
  if(heading >= 45 & heading < 90) led = indices[6];
  if(heading >= 90 & heading < 135) led = indices[5];
  if(heading >= 135 & heading < 180) led = indices[4];
  if(heading >= 180 & heading < 225) led = indices[3];
  if(heading >= 225 & heading < 270) led = indices[2];
  if(heading >= 270 & heading < 315) led = indices[1];
  if(heading >= 315 & heading < 360) led = indices[0];
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
