import pycom
import time
import urequests
import json

from network import WLAN

_headers = { "Content-Type": "application/json", "Accept": "application/json", "null": "null" }
_FINDhost = "http://ec2-54-209-226-130.compute-1.amazonaws.com"
_FINDpath = "/track"
_FINDport = 18003
_FINDgroup = "studiotest"
_FINDuser = "max"
_FINDloc = ""

wlan = WLAN()
    
def CleanMAC(_raw):
    return ':'.join('%02x' % b for b in _raw)

def ComposeAPreport():
    reportbody = "{\"group\":\""+_FINDgroup+"\",\"username\":\""+_FINDuser+"\",\"location\":\""+_FINDloc+"\",\"wifi-fingerprint\":["
    nets = wlan.scan()
    print("Found: ")
    print(len(nets))
    for ap in range(0,  len(nets)):
        signal = float(nets[ap].rssi) #not sure if this cast in necessary.
        prettymac = CleanMAC(nets[ap].bssid)
        _thisap = "{\"mac\":\"" + str(prettymac) + "\"," + "\"rssi\":" + str(signal) + "},"
        reportbody = reportbody + _thisap
    reportbody = reportbody[:-1] + "]}"#trim the last comma and close.
    print(reportbody)
    return reportbody
    
pycom.heartbeat(False)
while True:
    pycom.rgbled(0x007f00)
    nets = wlan.scan()
    print("Found: ")
    print(len(nets))
    _APs = ComposeAPreport()
    #make json data from APs...
    FINDpost = urequests.request("put",  _FINDhost + _FINDpath,  _APs,  None,  _headers,  None,  _FINDport)
    #response = urequests.get("http://ip.jsontest.com/")
    #_rjson = response.json()
    #print(_rjson["ip"])
    #response.close()
    pycom.rgbled(0x7f0000)
    time.sleep(2)
