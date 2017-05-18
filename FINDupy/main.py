import gc
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
import pathfinding
gc.collect()

_myID = binascii.hexlify(machine.unique_id())
_headers = { "Content-Type": "application/json;charset=UTF-8", "Accept": "application/json", "null": "null" }

_FINDhost = "http://ec2-54-209-226-130.compute-1.amazonaws.com:18003/track"
_FINDport = 18003
_FINDgroup = "studiotest"
_FINDuser = "pycom"
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
print(wlan.ifconfig())
coco = cocontroller.CoController()
coco._wait()

_STEPS = pathfinding.get_shortest_path(_START, _GOAL)
print(_STEPS)
#Setup methods

def _get_device_setup():
    SETUPget = urequests.get(_UTILITYhost, headers = _headers)
    _resp = SETUPget.json()
    print(_resp['status'])
    SETUPget.close()
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
    
#End Utility methods

def _start_return_nav():
    """
    """
    global _STEPS
    if _STEPS[0] == _GOAL:
        _STEPS = pathfinding.get_shortest_path(_START, _GOAL)
    else:
        _STEPS = pathfinding.get_shortest_path(_GOAL, _START)
    print(_STEPS)

def _update_nav_state(new):
    """
    """
    global _MOSTRECENT
    global _STEPS
    global _NEXTSTEP
    if len(new) > 1: #trim the first character
        if new[0] == "g":
            new = new[1:]
    if new != _MOSTRECENT: #if we've moved to a new gallery...
        if new in _STEPS:
                #we're on track, but are we getting colder or warmer?
            if new == _STEPS[-1]:
                coco._finish()
                print("Completed journey.")
                _start_return_nav()
                _locindex = _STEPS.index(new)
                _NEXTSTEP = _STEPS[_locindex + 1]
                print("Next step: ",  _NEXTSTEP)
                _heading = pathfinding._get_heading(_NEXTSTEP,  new)
                time.sleep(5)
                coco._update_direction(_heading)
            else:
                _locindex = _STEPS.index(new)
                _NEXTSTEP = _STEPS[_locindex + 1]
                print("Next step: ",  _NEXTSTEP)
                _heading = pathfinding._get_heading(_NEXTSTEP,  new)
                print("New heading: ",  _heading)
                coco._update_direction(_heading)
        else:
            if not pathfinding._is_in_immediate_neighbors(new, _MOSTRECENT):
               coco._wait() 
               return True
            #else:
                #user has left the path, so recalculate
            #    _STEPS = pathfinding.get_shortest_path(new, _GOAL)
            #   _locindex = _STEPS.index(new)
            #   _NEXTSTEP = _STEPS[_locindex + 1]
            #   _heading = pathfinding._get_heading(new, _NEXTSTEP)
            #   coco._update_direction(_heading)
        _MOSTRECENT = new
        return True
    else:
        return False

def _scan_and_post():
    pycom.rgbled(0x007f00)
    _APs = _compose_ap_report()
    #make json data from APs...
    payload = ujson.dumps({"group":"mia2f", "username":"pycom", "location":"test", "wifi-fingerprint":_APs})
    try:
        FINDpost = urequests.post(_FINDhost, data = payload, headers = _headers)
        if FINDpost:
            _rjson = FINDpost.json()
            print(_rjson['message'])
            locationname = _rjson['message'][18:]
            print(locationname)
            _update_nav_state(locationname)
            FINDpost.close()
            pycom.rgbled(0x7f0000)
        else:
            print("post failed")
    except MemoryError:
        print("Memory issue...")
        return False

pycom.heartbeat(False)
_start = time.ticks_ms()
time.sleep(2)
while True:
    if wlan.isconnected():
        if time.ticks_diff(_start, time.ticks_ms()) > _postinterval:
            _scan_and_post()
            _start = time.ticks_ms()
    else:
        print("Lost connection. Resetting!")
        wlan.connect('Mia-Guest', timeout=10000)
        
