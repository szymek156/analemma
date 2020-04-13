# Programming via ISP:

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
