# M5_mqtt_gps
M5 SIM7080G and M5StickC Plus2 IoT Cellular MQTT GPS Publisher

This is a minimal program for M5StickC Plus2 and the M5 Combo SIM7080G Cellular & GPS Module.
Note that the GPS and Cellular sections of the SIM7080G cannot be used simultaneously. Thus the program must turn off the GPS when sending MQTT messages.
All communication is via Serial2 port AT messsages between M5Stick and M5 SIM7080G, including building and sending MQTT messages.

By default, the MQTT message is hard coded to send "$GPS,UNITID,NMEALat,NMEALng,END" Change as desired!

IMPORTANT!!! The M5Stick ESP32 board library does not yet handle Espressif ESP32 Version 3.0 boards. Thus you MUST install the Espressif esp32 2.0.17 Board library via the Arduino IDE Boards Manager. Else you will get compile errors. This will hopefully be updated by M5 soon.

    Use M5StickC Plus2 and M5 SIM7080G GPS/Cellular combo module
    CAT-M to connect to the MQTT server and implement publishing GPS Location messages.
    Check the status through Serial and Screen. 
    When the MQTT connection is successful, check your MQTT Subscriber

Modify the .ino file with your MQTT server and if needed, userid/password.
You should use a Micro SIM card for the M5 SIM7080G module.  I used a SIMBASE IoT card, very low cost monthly and per MB. Be sure to pay attention to the direction of installation of the SIM card on the back of the M5 module.

The Cellular module does use a fair amount of power, so you may wish to power it using an external supply, either 5v via the M5Stick Plus module or via 12v on the M5 SIM plug connector. These should power both modules automatically. Note the internal battery of the M5Stick would give a very short run time by itself.
