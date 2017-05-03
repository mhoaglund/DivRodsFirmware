#Class to manage heading feedback via LED ring.
import ws2812
from machine import I2C
from bno055 import BNO055
class LightingManager:
      """
          Manages the visual feedback that is administered through our neopixel ring.
      """
      #Heading display stuff.
      
      def __init__(self):
          """
              Takes a ws2812 instance and wrangles heading updates.
          """
          
          #Using I2C to marshall our heading sensor.
          #That insane baudrate is how it is because the PyCom firmware
          #has yet to implement the clock stretching that our bno055 uses.
          #TODO: try with external crystal on, maybe we can use a higher rate.
          self.i2c = I2C(0)
          self.i2c.init(I2C.MASTER, baudrate=1000)
          self.bno = BNO055(self.i2c)
          self.ws2812 = ws2812.WS2812( ledNumber=8, dataPin='P11' )
          self.desired_relative_heading = 0 #desired heading from pathfinding algo
          self.offset_index = 0 #offset to rotate the compass and render the des. heading
          self.LED_MAP = [
            [0,1,2,3,4,5,6,7],
            [7,0,1,2,3,4,5,6],
            [6,7,0,1,2,3,4,5],
            [5,6,7,0,1,2,3,4],
            [4,5,6,7,0,1,2,3],
            [3,4,5,6,7,0,1,2],
            [2,3,4,5,6,7,0,1],
            [1,2,3,4,5,6,7,0]
          ]
          
      def ResampleHeading(self,  heading):
          """
              Takes a heading in degrees and determines which octet it's in.
          """
          oct = 0
          if heading >= 0 and heading < 45: 
            oct = 7
          if heading >= 45 and heading < 90: 
            oct = 6
          if heading >= 90 and heading < 135: 
            oct = 5
          if heading >= 135 and heading < 180: 
            oct = 4
          if heading >= 180 and heading < 225: 
            oct = 3
          if heading >= 225 and heading < 270: 
            oct = 2
          if heading >= 270 and heading < 315: 
            oct = 1
          if heading >= 315 and heading < 360: 
            oct = 0
          return oct
          
      def MapHeadingToLED(self,  heading,  indices):
        """
            Takes an int heading and returns one of the LEDs from the ring. DEPRECATED
        """
        led = 0
        if heading >= 0 and heading < 45: 
            led = indices[7]
        if heading >= 45 and heading < 90: 
            led = indices[6]
        if heading >= 90 and heading < 135: 
            led = indices[5]
        if heading >= 135 and heading < 180: 
            led = indices[4]
        if heading >= 180 and heading < 225: 
            led = indices[3]
        if heading >= 225 and heading < 270: 
            led = indices[2]
        if heading >= 270 and heading < 315: 
            led = indices[1]
        if heading >= 315 and heading < 360: 
            led = indices[0]
        return led
        
      def UpdateHeading(self,  desired):
          """
              Take in a desired heading and update compass
          """
          _headingactual = int(self.bno.euler()[0])
          _headingdesired = self.ResampleHeading(desired)
          
          target_led = self.MapHeadingToLED(_headingactual,  self.LED_MAP[_headingdesired])
          self.ShowHeading(target_led)
          
      def ShowHeading(self,  led):
          """
              Build a light cue showing a heading and send it off to be rendered.
          """
          cue = []
          for _led in range(0,  self.ws2812.ledNumber):
              cue.append((125, 85, 125))
          cue[led] = (255, 255, 125)
          self.RenderState(cue)
          
      def RenderState(self,  cue):
          self.ws2812.show(cue)
          
    
