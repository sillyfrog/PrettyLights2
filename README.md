# Pretty Lights 2

This is an app to manage RGB and PWM LED's from an ESP8266 board.

A demo of it managing a bunch of LEDs is available here: https://youtu.be/k2yD03bUgNY

A read only demo is available here: https://prettylightsdemo.sillyfrog.com/
This gives you a feel for what's possible, and how you configure each LED in your project.

It's designed to keep each "frame" in flash, and update all of the LED's as required, all managed via a web interface (UI).

Much of the work is setup client side, rather than "real time" on the chip, allowing for a lot more flexibility. The UI allows the creation, import and export of "Patters", which can then be applied to selected LED(s). The client side UI then packages everything up in a format that's easy for the ESP to read, and uploads it.

## Initial Setup

This was written on (PlatformIO)[https://platformio.org/] in VSCode, so would suggest doing the same.

The first thing to do before building and flashing the ESP is to create `/src/wificredentials.h` (use the `wificredentials.h.example` as a reference), and put in your WiFi setup. Then proceed to build and flash your ESP (see below for the hardware I have used).

Once the ESP is flashed, you need to get the web interface on to it. Figure out the IP address (either from your DHCP server, or the serial log output). You can then upload the `index.htm` file. You can also compress this file, (recommended), to take up less flash space, and reduce the size of transfers. To compress, run:
```
cd data
cat index.htm | gzip > index.htm.gz
```
Then to upload using curl, run:
```
curl 1.2.3.4/edit --form 'data=@index.htm.gz;filename=/index.htm.gz' -v
```
Where `1.2.3.4` in the IP address of your ESP. (Change both instances of `index.htm.gz` to `index.htm` if you have not compressed the file).

You should then be able to browse to the web interface of your ESP and start configuring things. (Please note, I have done all of my development and testing in (Firefox)[https://www.mozilla.org/en-US/exp/firefox/new/], so if you have issues, please try that first).

## Hardware

I have deployed all of this on the (LOLIN D1 Mini)[https://www.wemos.cc/en/latest/d1/d1_mini.html] (or the Pro), using a (TLC5947 based PWM board)[https://www.adafruit.com/product/1429], and SK6812 based RGB LED's. Typically I use a 12v rail for the TLC5947 so I can chain several LED's together in serial, and 5v for the RGB LED's and D1.

See the `rgb_pin_allocations` for the pins to use for RGB LED's (each pin can address how ever many RGB LED's, but I limit the length to limit interference). For the PWM LED controller, see `DN`, `CLK` and `pwm_lat_pin_allocations` for the LAT (if using 2 boards, the `DN` and `CLK` are shared).

On the TLC5947, I tie the `OE` pin to the `LAT` to help prevent flicker (although it does not help interference). For longer runs I also put inline a level shifter so the TLC5947 get 5v signals rather than 3.3v from the ESP. (I find the SK6812 is normally OK with a 3.3v signal).

## Flash File Format

The default flash filesystem is `LittleFS`. This stores both the actual UI webpage, and the patterns configuration. I found `LittleFS` to be faster that `SPIFFS` (and it's the preferred FS moving forward).

Everything is calculated in `frames`, where each frame is a discrete unit of time (typically 25 frames per second or 40ms per frame).

Patterns are split up between RGB and PWM. With every group of 10 files split into its own directory. Each file (by default) holds 25 frames. For example:
```
|-pwm00
 |-frames0000.dat
 |-frames0001.dat
 |-frames0002.dat
 |- ...
|-pwm01
 |-frames0010.dat
 |- ...
|-...
|-rgb01
 |-frames0000.dat
 |- ...
```
Each file is packed with the first frame of the first pattern, then the first frame of the next pattern etc, one after the other. For RGB that's 3-bytes per pixel, for PWM it's 2-bytes per pixel (but only 12bits is used).

So, the first few bytes in an RGB file would have the following:
```
000111222...
```
Where `000` is 3 bytes for the first frame of the first pattern, `111` is the 3 bytes for the first frame of the second pattern etc. Then once all the patterns for the first frame are stored, the next frame is packed (until `FRAMES_PER_FILE` is reached), then the next file is created etc.

I tried a number of different formats, including having a file per pattern, using JSON, single big file etc. They all had issues, but the biggest issue was generally speed. This format allows the chip to read a bunch of data at once, and use it, while opening no more than one file (per RGB/PWM). This makes editing a single pattern much harder, and adding more patterns is also a mess. However by moving all of this work to the browser the ESP never has to deal with the workload (nor memory limits it would otherwise hit).

For similar reasons, a `colorpwm.dat` and `colorrgb.dat` file is used to store the config of what pattern is assigned to which LED, and the starting colour of each LED. I tried ArduinoJSON, but the overhead for the ESP was too much, and it caused issues with WiFi connectivity, outright crashing etc.

## Web Interface Development

If you ever want to do development on the web interface, you can do this purely locally (much faster) before uploading to the chip. To do this, please see the top of the `webdev/app.py` file.

## Custom Adafruit TLC5947

You may note there is a custom Adafruit TLC5947 library included. This includes a `getPWM` function to allow getting the current value set on a PWM channel, used to prevent unnecessary updates to the TLC5947 (using the same memory space, rather than doubling up RAM requirements).
