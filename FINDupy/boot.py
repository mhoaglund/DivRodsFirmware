# boot.py -- run on boot-up
# can run arbitrary Python, but best to keep it minimal

import machine
import os
import time

uart = machine.UART(0, 115200)
os.dupterm(uart)

if machine.reset_cause() != machine.SOFT_RESET:
    from network import WLAN
    
    known_nets = [(('','')), (('', ''))]
    
    wl = WLAN()
    
    original_ssid = wl.ssid()
    original_auth = wl.auth()
    
    wl.antenna(WLAN.EXT_ANT)
    wl.mode(WLAN.STA)
    
    available_nets = wl.scan()
    nets = frozenset([e.ssid for e in available_nets])
    
    known_nets_names = frozenset([e[0] for e in known_nets])
    net_to_use = list(nets & known_nets_names)
    
    try:
        net_to_use = net_to_use[0]
        pwd = dict(known_nets)[net_to_use]
        sec = [e.sec for e in available_nets if e.ssid == net_to_use][0]
        wl.connect(net_to_use, (sec, pwd), timeout=10000)
    except:
        wl.init(mode=WLAN.AP, ssid=original_ssid, auth=original_auth, channel=6, antenna=WLAN.INT_ANT)

time.sleep(1)
