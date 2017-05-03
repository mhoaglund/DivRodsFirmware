import pycom
import time
import urequests
import ujson
from network import WLAN
import _thread
import lightfeedback
import pathfinding

_headers = { "Content-Type": "application/json;charset=UTF-8", "Accept": "application/json", "null": "null" }
_FINDhost = "http://ec2-54-209-226-130.compute-1.amazonaws.com:18003/track"
_FINDport = 18003
_FINDgroup = "studiotest"
_FINDuser = "pycom"
_FINDloc = "test"
_postinterval = 2000 #how long to wait between nav calls

wlan = WLAN()
lightmgr = lightfeedback.LightingManager()

def GetDeviceSetup():
    return 0
    
def RetrieveFloorMap():
    return 0
    
def CleanMAC(_raw):
    return ':'.join('%02x' % b for b in _raw)
    
def ComposeAPreport_obj():
    aps = []
    nets = wlan.scan()
    for ap in range(0,  len(nets)):
        prettymac = CleanMAC(nets[ap].bssid)
        apjson = {"mac": prettymac, "rssi": nets[ap].rssi}
        aps.append(apjson)
    return aps
    
pycom.heartbeat(False)

def Feedback_Thread(rate, id):
    while True:
        _heading = 0
        lightmgr.UpdateHeading(int(_heading))
        time.sleep_ms(rate)

_thread.start_new_thread(Feedback_Thread, (80, 0))

def ScanAndPost():
    pycom.rgbled(0x007f00)
    nets = wlan.scan()
    print("Found: ")
    print(len(nets))
    _APs = ComposeAPreport_obj()
    #make json data from APs...
    payload = ujson.dumps({"group":"mia2f", "username":"pycom", "location":"test", "wifi-fingerprint":_APs})
    FINDpost = urequests.post(_FINDhost,  data = payload,  headers = _headers)
    _rjson = FINDpost.json()
    print(_rjson['message'])
    FINDpost.close()
    pycom.rgbled(0x7f0000)

_start = time.ticks_ms()

while True:
    if wlan.isconnected():
        if time.ticks_diff(_start,  time.ticks_ms()) > _postinterval:
            ScanAndPost()
            _start = time.ticks_ms()
    else:
        #TODO try to regain connection
        pycom.rgbled(0x00007f)
