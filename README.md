# Programming via ISP:

https://www.iot-experiments.com/arduino-pro-mini-1mhz-1-8v/
http://www.engbedded.com/fusecalc
https://andreasrohner.at/posts/Electronics/How-to-fix-device-permissions-for-the-USBasp-programmer/

## Check validity
sudo ./avrdude -c usbasp -p atmega328p -C ./avrdude.conf

## Disable brown out detection, reduce clock to 1MHz:
sudo ./avrdude -c usbasp -p atmega328p -C ./avrdude.conf -U lfuse:w:0x62:m -U hfuse:w:0xdb:m -U efuse:w:0xff:m 

Set terminal baud rate to 1200 (/8 times slower)

## Default pro mini 3.3V 8MHz version config 
sudo ./avrdude -c usbasp -p atmega328p -C ./avrdude.conf -U lfuse:w:0xff:m -U hfuse:w:0xda:m -U efuse:w:0xfd:m 

## Change permissions for ISP (lazy ass):
lsusb
sudo chmod 666 /dev/bus/usb/002/121

#To be verified
 CLKPS3..0: Clock Prescaler Select Bits 3 - 0

# brown out disabled no clock division, bootloader untouched 
avrdude: safemode: Fuses OK (E:FE, H:D9, L:FF)

# internal clock source selected, division enabled
avrdude: safemode: Fuses OK (E:FE, H:D9, L:62)


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
- Freq is set to 1MHz
- Power led is removed
- Voltage regulator is removed
- Everything is grounded
- RTC clock is set to UTC
- RTC has backup battery (optional)


internal osc:
avrdude: safemode: Fuses OK (E:FF, H:DB, L:E2)

internal osc, div by 8
avrdude: safemode: Fuses OK (E:FF, H:DB, L:62)


str94