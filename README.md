## What this is:

This project is for attaching a **rd-03d** from AI-Thinker to homeassistant via mqtt to control e.g. lights depending on the location of a person in a room

## You will need:

- Any ESP board, I used an ESP 32, for an ESP 8288 you will have to adapt the code slightly to use the right Libraries
- A rd-03d radar or similar
- A Homeassistant instance
- A MQTT server connected to HA (or the plugin in HA)
- VSCode with Platformio

## Steps:

1. rename SECRETS_example.h to SECTRETS.h and fill in your details
2. copy an svg image of your roomplan into /data instead of Board.svg.
   If you want another format/name you need to change the name in script.js(img.src) and in main.cpp(setup).
   Adjust the aspect ratio of the canvas in index.html to your image. 1px should be one cm to get the scaling right.
   To check if this works you can open index.html locallly
3. Name your Zones in main.cpp.
   If you want more zones, add lines to preset.json, add a case for editing to script.js and increase MAX_SPACES.
   You can later change those names in HA too
4. Attach the appropriate connections to the sensor
   - the sensor needs a 5v supply, this is why I opted for a ESP32 Devkit, as VIN is the 5v from USB
5. change the RX and TX definitions to the pins you attached the Sensor to.
   Choose a Hardware serial for this.
6. in Platformio, choose the option project Tasks>your device>platform>Build Filesystem
   and then Upload Filesystem to upload the littlefs files for the Webserver
7. upload the code by choosing project Tasks>your device>general>upload
8. When plugged in, the Sensors should automatically load into HA
9. now go to the ip adress of the esp (is printed when using the serial monitor, or check your modem)
   and adjust your zones and sensor location by using 1...7 to select the zone to edit by dragging the mouse and space to deselect.
   To change the sensor location press s, to rotate it press a and d to finish placement press space.
   When not in edit mode click the mouse to get the current location the sensor sees.
   The canvas is scaled to fill the height of the screen, but if you drag your mouse outside, you can also create zones outside of your room (e.g. for the door).
   Zone 1 is treated like any other, but is displayed more transparent as I use it to check the entire room.
10. in main.cpp(create MQTT message and discoverymessage) you can easily add more sensors like the x and y coordinates or the speed

## Quirks:

Sometimes the sensor looses you for a short moment, be aware of this and handle it accordingly.

The location the sensor reports is often the bodypart closest to the sensor. Depending on the accuracy you need you will have to account for this in your zones.

Sometimes the sensor gets confused and jumps a bit for a second, ensure your zones are large enough for this.

You cannot have the sensor near/behind big metal/electronic things like screens if you want accuracy.

There is a minimum distance of around 60cm for detection.

in theory you have 120° of vision - but the extremes are not that reliable.

This sensor is closed source, meaning there is almost no documentation. This code works, but changing something can be challenging (like adjusting the reporting frequency or single/multitarget detection)

Mount it somewhat securely, it does some sort of calibration at the start, and especially a changing angle can wildly disturb where the sensor tracks you to be

## Sources used:

[https://docs.ai-thinker.com/\_media/rd-03d_multi-target_trajectory_tracking_user_manual.pdf](https://docs.ai-thinker.com/_media/rd-03d_multi-target_trajectory_tracking_user_manual.pdf)

[https://www.youtube.com/watch?v=cSI9vedf870](https://www.youtube.com/watch?v=cSI9vedf870)

[https://core-electronics.com.au/guides/arduino/detect-and-track-humans-with-mmwave-radar-on-an-arduino/](https://core-electronics.com.au/guides/arduino/detect-and-track-humans-with-mmwave-radar-on-an-arduino/)

[https://openelab.io/products/ai-thinker-rd-03d-24g-millimeter-wave-radar?srsltid=AfmBOopjVVYwQFmaWzvRLUTZZvhMpmuUYHHag7MReVSDgZymlJR7VrHP](https://openelab.io/products/ai-thinker-rd-03d-24g-millimeter-wave-radar?srsltid=AfmBOopjVVYwQFmaWzvRLUTZZvhMpmuUYHHag7MReVSDgZymlJR7VrHP)

[https://en.ai-thinker.com/pro_view-140.html](https://en.ai-thinker.com/pro_view-140.html)
