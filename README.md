# LED matrix MQTT monitor (MD_Parola)
Author: Marc Staehli, 2019

[![LED matrix MQTT monitor](https://github.com/3KUdelta/MDparola_MQTT_monitor/blob/master/pictures/IMG_3180.JPG)](https://github.com/3KUdelta/MDparola_MQTT_monitor)

see the video: https://github.com/3KUdelta/MDparola_MQTT_monitor/blob/master/pictures/IMG_3175.mp4

Main version:
- NodeMCU V3 - using WiFi to get NTP Time and connect to MQTT broker
- 12 LED matrix units (MAX7219)--> 3 LED units FC16 (https://www.aliexpress.com/item/32648450356.html)
- 1 FC16 unit on the left (yellow/green) showing exact time taken from NTP servers
- 2 FC16 units on the right (blue) scrolling MQTT messages
- PIR sensor activating display for 2 minutes after detection
- Using the MD_Parola libraries - thank you for this excellent work! (https://github.com/MajicDesigns/MD_Parola)

The data is coming from some of my other projects: Solar Weather Station: https://github.com/3KUdelta/Solar_WiFi_Weather_Station and my Crocodile Pool Sensor: https://github.com/3KUdelta/Crocodile-Solar-Pool-Sensor. 
I am using an Orange Pi Zero with Mosquitto as a MQTT broker.

Print the boxes yourself: https://www.thingiverse.com/thing:2811071, https://www.thingiverse.com/thing:1374504

[![LED matrix MQTT monitor](https://github.com/3KUdelta/MDparola_MQTT_monitor/blob/master/pictures/LED_parola_MQTT_monitor.png)](https://github.com/3KUdelta/MDparola_MQTT_monitor)

[![LED matrix MQTT monitor](https://github.com/3KUdelta/MDparola_MQTT_monitor/blob/master/pictures/IMG_3172.JPG)](https://github.com/3KUdelta/MDparola_MQTT_monitor)

[![LED matrix MQTT monitor](https://github.com/3KUdelta/MDparola_MQTT_monitor/blob/master/pictures/IMG_3176.JPG)](https://github.com/3KUdelta/MDparola_MQTT_monitor)

[![LED matrix MQTT monitor](https://github.com/3KUdelta/MDparola_MQTT_monitor/blob/master/pictures/IMG_3177.JPG)](https://github.com/3KUdelta/MDparola_MQTT_monitor)
