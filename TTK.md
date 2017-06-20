Things to Know (WiPy 2):

HTTP:
Getting a WiPy 2.0 to do a simple HTTP request is easy, and there are a handful of ways to do it. However, none of the ways that involve a nice API are documented in any cogent way.

https://github.com/micropython/micropython-lib/blob/master/urequests/urequests.py

If you just throw that in your PyCom device's lib folder as-is, it won't work or give you an error you can understand/use/is documented. The urequests library imports and uses usocket, a vanilla micropython library. You could go get that and throw it on your device, but your device already has a socket library in its firmware, so why not use that? The built-in pycom module is just called "socket". urequests can be updated to conditionally try to load that at first.
---
AP Scanning Performance:
The ESP32 narrowly beats the Photon's broadcom whatever out of the box as far as its ability to scan for APs. Comparing the two out of the box using onboard antennas, the ESP32 on the WiPy 2.0 consistently sees about 20% more APs. The Photon's wifi.scan() takes ~3 seconds for some reason- the same firmware function on the WiPy 2.0 is almost instantaneous.

With a 6DBi antenna, the WiPy 2.0 almost competes with a typical android tablet, making it suitable for use with FIND.

Other tidbits:
-The WiPy's OS returns BSSIDs (MAC addresses) as python byte literals.