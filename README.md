# ESP32 Bidirectional MIDI to OSC module

A bidirectional Hardware MIDI system for displaying/controlling MIDI Controls via Touch OSC 

This project uses an ESP32 connected to the MIDI IN and OUT ports on a Kork Monologue (however this is designed to be genral purpose for any MIDI device). The ESP32 has as self hosted web based UI that alows you to dynamicly map any MIDI control to any OSC control on the fly. 

# ESP32 Set up and Pinout

<img src="https://github.com/leonyuhanov/MIDItoWIFINode/blob/master/midi-labeled.png" width="400" />

For the MIDI OUT port on the device:

* PIN 4 is the DATA OUT pin that connects into Pin 25 on the ESP32
* PIN 2 conects to GND of the ESP32

For the MIDI IN port on the device:

* PIN 4 is the DATA IN pin that connects into Pin 26 on the ESP32
* PIN 2 conects to GND of the ESP32

You will need to upload the contents of the DATA folder to the ESP32 SPIFFS using the ESP32 SKetch Data Upoad tool https://github.com/me-no-dev/arduino-esp32fs-plugin

Set up your WIFI network defails int he code/compile and upload. You will need to modify the IP address detais for your network and set up Touch osc acordingly

Then compile and upload via the arduino ide. The serial monitor will outout the devices IP address
