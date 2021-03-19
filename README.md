# What is it and features
Automation of Analemma pinhole camera. 
Analemma is is an astronomical phenomena, it's an eitght like shape sun traces over the sky, during a year. 
It's not visible by human eye, to capture it visually it's requred to take a photo of the sun every single day over the year, at the same time, from the exact same place on the Earth. Having 365 photographies of the sun and overlap them alltoghether "analemma" shape will emerge.
Great animation from dozens of such photos was already done and shared here: 
https://www.youtube.com/watch?v=ZS2FvljQXsk
https://www.youtube.com/watch?v=ACerzpR-bbc

Facts:
* Highest point of the "8" is the summer solistice
* Lowest point of the "8" is the winter solistice
* Middle intersection of "8" are March and September equinoxes

This project is an automation of pinhole camera, where light sensitive paper is installed in a can and light is reaching it by small hole in it.
Refer to solarigraphy technique https://en.wikipedia.org/wiki/Solarigraphy

Requirements:
* Extremly low power comsumption (installation has to last for over a year outdoor) (WIP)
* Weather resistant (Done!)
* Driven on batteries (Done! But look at point 1)
* Accurate time keeping (Done!)
* Time set by GPS at the "bringup" phase of the system (Done!)
* Possibility of defining exact date and time of exposition duration (Done!)
* Logging of shutter events for debugging purpouses  (Done!)

## HW
SG90 servo to open and close the shutter
DS3231 RTC for keeping time accurate - it has temperature time compensation and it's baked up by coin battery.
Arduino pro mini 3.3V 8MHz for managing all of it.
Supplied with 4 AAA batteries

## Features
* Arduino CPU clockrate is reduced to 1MHz,
* BOD (brown out detection) is set to 1.5V,
* Removed power LED and voltage regulator to minimize power comsumption.
* Easy exposition configuration (look into the code). It is possible to setup custom exposition times, for example for brightening image, and for solistices/equinoxes (then full sun trace will be visible on the image not the on point from given hour)
* Bringup project to setup RTC time from GPS

## Code flow
Arduino most of the time is in deep sleep mode, waiting for interrupt from RTC. Upon wakeup, shutter is opened for given period of time, then closed and again falls into deep sleep mode.



# Personal Notes
## Programming via ISP:

https://www.iot-experiments.com/arduino-pro-mini-1mhz-1-8v/
http://www.engbedded.com/fusecalc
https://andreasrohner.at/posts/Electronics/How-to-fix-device-permissions-for-the-USBasp-programmer/

## Check validity
sudo ./avrdude -c usbasp -p atmega328p -C ./avrdude.conf

## Disable brown out detection:
sudo ./avrdude -c usbasp -p atmega328p -C ./avrdude.conf -U lfuse:w:0xff:m -U hfuse:w:0xda:m -U efuse:w:0xfe:m 

Clock reduction (div 8) is done at runtime 
Set terminal baud rate to 1200 (/8 times slower)

## Default pro mini 3.3V 8MHz version config 
sudo ./avrdude -c usbasp -p atmega328p -C ./avrdude.conf -U lfuse:w:0xff:m -U hfuse:w:0xda:m -U efuse:w:0xfd:m 

## Change permissions for ISP (lazy ass):
lsusb
sudo chmod 666 /dev/bus/usb/002/121

Add 1MHz version to IDE:
Add  to 
~/arduino-1.8.10/hardware/arduino/avr/boards.txt

######################################################################
promini1MhzInt.name=Arduino Pro Mini (1MHz internal, 1.8V)
promini1MhzInt.upload.tool=avrdude
promini1MhzInt.build.board=AVR_PRO
promini1MhzInt.build.mcu=atmega328p
promini1MhzInt.build.f_cpu=1000000L
promini1MhzInt.build.core=arduino
promini1MhzInt.build.variant=eightanaloginputs


# System check:
- Schedule is validated
- Production SW is uploaded (blinks twice at startup)
- BOD is disabled, or set to 1.5v
- Freq is set to 1MHz (there should be serial output at rate 1200)
- Power led is removed
- Voltage regulator is removed
- Everything is grounded
- RTC clock is set to UTC
- RTC has backup battery (optional)
