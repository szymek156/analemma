# Programming via ISP:

https://www.iot-experiments.com/arduino-pro-mini-1mhz-1-8v/
http://www.engbedded.com/fusecalc
https://andreasrohner.at/posts/Electronics/How-to-fix-device-permissions-for-the-USBasp-programmer/

## Check validity
sudo ./avrdude -c usbasp -p atmega328p -C ./avrdude.conf

## Disable brown out detection, reduce clock to 1MHz:
sudo ./avrdude -c usbasp -p atmega328p -C ./avrdude.conf -U lfuse:w:0x62:m -U hfuse:w:0xdb:m -U efuse:w:0xff:m 

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
