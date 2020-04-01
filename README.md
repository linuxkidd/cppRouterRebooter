# cppRouterRebooter
Sonoff S31 based configurable Router / Modem rebooter with MQTT


### Features
* Power cycle's connected device on internet failure
* Publishes status udpates via MQTT and local web page

### WiFi Details

**WiFi SSID:** `RouterRebooter_XXXXXX`
**WiFi Password:** `routerrebooter`
**URL:** `192.168.4.1`

After it joins your wifi, find its IP and load it in your browser.

### Configurability
* MQTT Server and Topic base
* Test server and port
* Test interval
    * seconds between test attempts
* Failure count before reset 
    * doesn't reboot on first failure )
* Success count to reset failure status
    * allow recovery from temporary outage without restarting
* Minimum device on time
    * prevent power cycling before device would normally have been online
* Device off-time
    * How long to leave the device off to ensure a proper reset

### MQTT Published topics:
* RouterRebooter/available
    * online
    * offline
* RouterRebooter/status
    * `{"lastms": 20, "countFail": 0, "countSuccess": 0, "routerReset": 0, "epoch": 1585687912, "ip": "192.168.1.2", "rssi": "-34 dBm" }`
    * lastms = Last connection success time in milliseconds
    * countFail = How many attempts have failed in this session - will reset to 0 if connectivity returns
    * countSuccess = How many attempts have succeeded in this session - will only increment if previous failures detected and not reset to 0
    * routerReset = If the router is getting ready to be rebooted
    * epoch = Unix Epoch Time
    * ip = IP address of the unit
    * wifi = WiFi Network Name to which the device is connected
    * rssi = WiFi Signal Strength for the device

### MQTT Subscribed topics:
* RouterRebooter/switch/cmd
    * Set to 0 to reboot the connected device, will auto-turn back on
* RouterRebooter/esp/cmd
    * Set to 0 to reboot the ESP module
