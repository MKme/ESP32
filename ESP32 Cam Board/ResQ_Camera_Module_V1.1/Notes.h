// Notes.h tab in Arduino IDE is only for comments and references!

// based on RandDruid/esp8266-deauth (MIT) https://github.com/RandDruid/esp8266-deauth
// inspired by kripthor/WiFiBeaconJam (no license) https://github.com/kripthor/WiFiBeaconJam
// https://git.schneefux.xyz/schneefux/jimmiejammer/src/master/jimmiejammer.ino
// requires SDK v1.3: install esp8266/Arduino from git and checkout commit 1c5751460b7988041fdc80e0f28a31464cdf97a3
// Modified by M. Ray Burnette for publication as WiFi Sniffer 20161013 (MIT)
// Original license by Ray published under MIT license https://opensource.org/licenses/MIT
// Source https://www.hackster.io/rayburne/esp8266-mini-sniff-f6b93a
// Modified and re-devolped for ResQ by Eric @mkmeorg www.mkme.org
// Republished github.com/mkme/resq and Hackaday IO project
// Licensed under a Creative Commons Attribution-NonCommercial 4.0 International License. https://creativecommons.org/licenses/by-nc/4.0/

//ESPCam based from Rui (No License) https://randomnerdtutorials.com/esp32-cam-take-photo-save-microsd-card/

/*
   Arduino 1.6.12 on Linux Mint 17.3
    Sketch uses 227,309 bytes (21%) of program storage space. Maximum is 1,044,464 bytes.
    Global variables use 45,196 bytes (55%) of dynamic memory, leaving 36,724 bytes for local variables. Maximum is 81,920 bytes.

*/

/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-take-photo-save-microsd-card
  
  IMPORTANT!!! 
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/