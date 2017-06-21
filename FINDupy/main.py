import gc
import math
import pycom
gc.collect()
import machine
gc.collect()
import time
gc.collect()
import urequests
gc.collect()
import ujson
gc.collect()
import cocontroller
gc.collect()
import binascii
gc.collect()
from network import WLAN
gc.collect()

_myID = binascii.hexlify(machine.unique_id())
_headers = { "Content-Type": "application/json;charset=UTF-8", "Accept": "application/json", "null": "null" }

_FINDhost = "http://ec2-54-209-226-130.compute-1.amazonaws.com:18003/track"
_FINDport = 18003
_FINDgroup = "mia2f"
_FINDuser = str(_myID)
_FINDloc = "test"
_postinterval = 2000 #how long to wait between nav calls
_MOSTRECENT = ""
_START = '275'
_GOAL = '260'
_STEPS = []
_RECENTSTEP = -1
_NEXTSTEP = -1

_UTILITYhost = ""

wlan = WLAN()
wdt = machine.WDT(timeout=12000)
print(wlan.ifconfig())
coco = cocontroller.CoController()
coco._wait()

def _get_device_setup():
    SETUPget = urequests.get(_UTILITYhost, headers = _headers)
    _resp = SETUPget.json()
    print(_resp['status'])
    SETUPget.close()
    return 0
    
def _get_path(location,  destination):
    PATHget = urequests.get(_UTILITYhost, headers = _headers)
    return 0

def _get_floor_map():
    return 0

#End Setup methods

#Utility methods

def _cleanmac(_raw):
    return ':'.join('%02x' % b for b in _raw)

def _compose_ap_report():
    aps = []
    nets = wlan.scan()
    for ap in range(0, len(nets)):
        prettymac = _cleanmac(nets[ap].bssid)
        apjson = {"mac": prettymac, "rssi": nets[ap].rssi}
        aps.append(apjson)
    return aps

def _get_heading(_location, _destination):
    """
        Return int representing heading in degrees from one node to another.
        :param destination: xy coord array
        :param location: xy coord array
    """
    #angle = math.degrees(math.atan2(_destination[1] - _location[1], _destination[0] - _location[0]))
    angle = math.degrees(math.atan2(_location[1] - _destination[1], _destination[0] - _location[0]))
    #bearing1 = (angle + 360) % 360
    bearing2 = (90 - angle ) % 360
    return int(bearing2)
#End Utility methods

def _update_nav_state(new):
    """
    """
    global _MOSTRECENT
    global _STEPS
    global _NEXTSTEP
    if len(new) > 1: #trim the first character
        coco._print(new)
        if new[0] == "g":
            new = new[1:]
    if new != _MOSTRECENT: #if we've moved to a new gallery...
        if new in _STEPS:
                #we're on track, but are we getting colder or warmer?
            if new == _STEPS[-1]:
                coco._finish()
                print("Completed journey.")
                #new journey
                _locindex = _STEPS.index(new)
                _NEXTSTEP = _STEPS[_locindex + 1]
                print("Next step: ",  _NEXTSTEP)
                _heading = _get_heading(_NEXTSTEP,  new)
                time.sleep(5)
                coco._update_direction(_heading)
            else:
                _locindex = _STEPS.index(new)
                _NEXTSTEP = _STEPS[_locindex + 1]
                print("Next step: ",  _NEXTSTEP)
                _heading = _get_heading(_NEXTSTEP,  new)
                print("New heading: ",  _heading)
                coco._update_direction(_heading)
        else:
            return True
            #get new path! user has left the path.
        _MOSTRECENT = new
        return True
    else:
        coco._print("Empty Location.")
        return False

def _scan_and_post():
    pycom.rgbled(0x007f00)
    _APs = _compose_ap_report()
    #make json data from APs...
    payload = ujson.dumps({"group":_FINDgroup, "username":_FINDuser, "location":"test", "wifi-fingerprint":_APs})
    try:
        FINDpost = urequests.post(_FINDhost, data = payload, headers = _headers)
        if FINDpost:
            _rjson = FINDpost.json()
            locationname = _rjson['location']
            _update_nav_state(locationname)
            pycom.rgbled(0x7f0000)
        else:
            coco._print("Post failed.")
    except urequests.URLError as e:
        pycom.rgbled(0x2f2f2f)
        coco._print("HTTP Error.")
        time.sleep(0.5)
        print(e)
        return False

pycom.heartbeat(False)
_start = time.ticks_ms()
time.sleep(2)
_get_path("254",  "275")
while True:
    if wlan.isconnected():
        if time.ticks_diff(_start, time.ticks_ms()) > _postinterval:
            _scan_and_post()
            _start = time.ticks_ms()
            wdt.feed()
    else:
        time.sleep(0.5)
        wdt.feed()
