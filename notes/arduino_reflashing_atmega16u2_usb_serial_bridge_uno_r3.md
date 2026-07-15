- Instructions on how to fix the USB chip on Arduino UNOr3 for official boards
- This chip is actually another Atmega chip called the ```Atmega16u2```
- It is programmed to act as a usb to serial bridge for the atmega328p on the UNO R3 since it has hardware USB

- NOTE: Other boards especially clones use a different IC like the ```CH340``` is a dedicated cheaper USB to UART bridge which doesn't rely on being programmed and instead requires a driver from ```WCH```

Command to upload the USB hex file to Atmega16u2 is:

```bash
./avrdude.exe
    -C ../etc/avrdude.conf 
    -p m16u2 
    -c usbasp -P usb 
    -U flash:w:./Genuino-COMBINED-dfu-usbserial-atmega16u2-Uno-R3.hex 
    -U lfuse:w:0xFF:m -U hfuse:w:0xD9:m -U efuse:w:0xF4:m -U lock:w:0x0F:m
```

Download for other hex files is at:
- https://github.com/arduino/ArduinoCore-avr/tree/master/firmwares/atmegaxxu2

Source for correct FUSE settings is at:
- https://forum.arduino.cc/t/trying-to-program-a-factory-atmega16u2/539254/2
