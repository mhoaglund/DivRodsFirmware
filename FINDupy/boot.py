# boot.py -- run on boot-up
# can run arbitrary Python, but best to keep it minimal

import machine
import os
import time

uart = machine.UART(0, 115200)
os.dupterm(uart)

if machine.reset_cause() != machine.SOFT_RESET:
    from network import WLAN

    known_secure_nets = {
        # SSID : PSK (passphrase)
        'Disney': 'eaglehouse',
        'StudioNorth': 'northernlights', 
    } # change this dict to match your WiFi settings

    wl = WLAN()

    original_ssid = wl.ssid()
    original_auth = wl.auth()

    wl.antenna(WLAN.EXT_ANT)
    wl.mode(WLAN.STA)
    
    for ssid, bssid, sec, channel, rssi in wl.scan():
        # Note: could choose priority by rssi if desired
        if ssid == 'Mia-Guest':
            wl.connect('Mia-Guest', timeout=10000)
            break
        try:
            wl.connect(ssid, (sec, known_secure_nets[ssid]), timeout=10000)
            break
        except KeyError:
            pass # unknown SSID
    else:
        wl.init(mode=WLAN.AP, ssid=original_ssid, auth=original_auth, channel=6, antenna=WLAN.INT_ANT)

time.sleep(1)
