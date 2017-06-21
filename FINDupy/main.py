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
_UTILITYhost = "http://node-express-env.afdmv4mhpg.us-east-1.elasticbeanstalk.com"
_FINDport = 18003
_FINDgroup = "mia2f"
_FINDuser = str(_myID)
#_FINDuser = "meeeee"
_FINDloc = "test"
_postinterval = 2000 #how long to wait between nav calls
_MOSTRECENT = ""
_START = '275'
_END = '260'
_GOAL = '260'
_STEPS = []
_RECENTSTEP = -1
_NEXTSTEP = -1



wlan = WLAN()
wdt = machine.WDT(timeout=12000)
print(wlan.ifconfig())
coco = cocontroller.CoController()
coco._wait()

def _get_path(location,  destination):
    global _STEPS
    try:
        qparams = {"deviceid":_FINDuser, "start": location,  "end": destination}
        path_get = urequests.get("http://node-express-env.afdmv4mhpg.us-east-1.elasticbeanstalk.com/path", qparams,  headers = _headers)
        if path_get:
            path_json = path_get.json()
            _STEPS = []
            print(len(path_json["journey"]))
            for step in range(0, len(path_json["journey"])):
                _jsonstep = path_json["journey"][step]
                _step = (_jsonstep["room"], _jsonstep["coords"][0], _jsonstep["coords"][1])
                _STEPS.append(_step)
            print(_STEPS)
        else:
            coco._print("Post failed.")
            print("getting path failed")
    except urequests.URLError as e:
        coco._print("HTTP Error getting path.")
        print(e)

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
    angle = math.degrees(math.atan2(_location[1] - _destination[1], _destination[0] - _location[0]))
    bearing2 = (90 - angle) % 360
    return int(bearing2)
#End Utility methods

def _update_nav_state(new):
    """
    """
    global _MOSTRECENT
    global _STEPS
    global _NEXTSTEP
    global _GOAL
    _ontrack = False
    _stepindex = -1
    _onstep = []
    if len(new) > 1: #trim the first character
        coco._print(new)
        if new[0] == "g":
            new = new[1:]
        for ind in range(0, len(_STEPS)):
            if new == _STEPS[ind][0]:
                _ontrack = True
                _stepindex = ind
                _onstep = _STEPS[ind]
        if _ontrack:
            if _stepindex == len(_STEPS)-1:
                coco._finish()
                print("Completed journey.")
                #Swapping back and forth for now.
                #Get a new path to keep going.
                if(_GOAL == _START):
                    _GOAL = _END
                else:
                    _GOAL = _START
                _get_path(new, _GOAL)
                time.sleep(3)
            else:
                _nextstep = _STEPS[_stepindex + 1]
            _curr_coord = [_onstep[1], _onstep[2]]
            _nextstep_coord = [_nextstep[1], _nextstep[2]]
            _heading = _get_heading(_curr_coord,  _nextstep_coord)
            coco._update_direction(_heading)
        else:
            coco._wait()
            _get_path(new, _GOAL)
            time.sleep(1)
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
            print(locationname)
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
_get_path(_START, _GOAL)
while True:
    if wlan.isconnected():
        if time.ticks_diff(_start, time.ticks_ms()) > _postinterval:
            _scan_and_post()
            _start = time.ticks_ms()
            wdt.feed()
    else:
        time.sleep(0.5)
        wdt.feed()
