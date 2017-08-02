OTAfanctrl

A wifi-enabled controller integrated with a ceiling fan's remote control and the zwhome automation system.

Hardware used:
- NodeCMU controller
- Fan-HD type remote control for Hampton Bay ceiling fans
- CD4066B CMOS Quad Bilateral Switch
- IIC I2C Logic Level Converter Bi-Directional Module

Software Used (Note: Versions may have changed)
- Arduino development environment for ESP8266 (see https://github.com/esp8266/Arduino)
- aREST library, which implements a REST API for Arduino & the ESP8266 WiFi chip (see https://github.com/marcoschwartz/aREST)

Work Flow

1. Install the development environment

2. Install the aREST library

3. Change the OTAfanctrl.ino file accordingly. I strongly suggest these:

   a. "your_ssid" to your wifi ssid

   b. "your_password" to your wifi password

4. Wire up the components as shown in schematic.pdf in this repository. There's nothing tricky about the construction of it. I soldered wires to existing solder points associated with the button contacts of the fan remote control after removing its board from the case.

5. Program the NodeCMU module.

6. Reset the NodeCMU module and it should run the program. If you send it a URI of this form:

	http://IP.Address.Of.Device/

You should see a JSON response of this form:

    {"variables": {"state": 1, "level": 25}, "id": "1", "name": "fanctrl", "hardware": "esp8266", "connected": true}
	
where--

    "state" is the variable name for the speed of the fan. Its values are

		0=off, 1=low, 2=medium, and 3=high
        
    "level" is the variable name for the corresponding percentage of fan speed. More on this later.

    "id" corresponds to the ID of the device

    "1" will be whatever value you gave it in the program (e.g. rest.set_id("1"))

    "name" is the name of the device

    "bedroom.fan" will be whatever name you gave it in the program (e.g. rest.set_name("bedroom.fan"))
    
NOTE: At this time, there is a bug in aREST, such that sending the URI with a browser will crash the program. The reason appears to be related to the amount of header information that is sent back by the browser. I have had no problems using libcurl in my programs or curl from the command line (see https://curl.haxx.se/).
Command line example:

	curl http://IP.Address.Of.Device/zwh?params=on,25
    
To turn off the fan, send this URI

    http://IP.Address.Of.Device/zwh?params=off

To switch the speed of the fan, send this URI

    http://IP.Address.Of.Device/zwh?params=on,<percentage>
   
In the URI, <percentage> should be replaced by a number from 1 to 100, corresponding to these fan speeds:

    Numbers from 1 to 30 = low
    Numbers from 31 to 60 = medium
    Numbers from 61 to 100 = high
    
Why did I do it this way? This device was intended to work with the Amazon Echo via a Hues Emulator Bridge (https://github.com/bwssytems/ha-bridge). This allows for voice commands, such as:

    "Alexa, turn on the bedroom fan"

    "Alexa, turn on the bedroom fan to 50 percent"

    "Alexa, turn off the bedroom fan"
    
NOTE: In addition to sending the JSON response, it will also send a request to the zwhome web server (e.g. using bedroom.fan as the name and low as the current fan speed) in the form of:

http://IP.Address.Of.ZwhomeServer:8000/devices/bedroom.fan?status=1&level=25

