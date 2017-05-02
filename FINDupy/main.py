import pycom
import time
import urequests
import ujson
from network import WLAN

_headers = { "Content-Type": "application/json;charset=UTF-8", "Accept": "application/json", "null": "null" }
_FINDhost = "http://ec2-54-209-226-130.compute-1.amazonaws.com:18003/track"
_FINDport = 18003
_FINDgroup = "studiotest"
_FINDuser = "pycom"
_FINDloc = "test"

wlan = WLAN()

def GetDeviceSetup():
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
while True:
    if wlan.isconnected():
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
        #response.close()
        pycom.rgbled(0x7f0000)
        time.sleep(2)
    else:
        pycom.rgbled(0x00007f)
