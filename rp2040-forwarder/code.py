# CircuitPython USB MIDI to serial MIDI converter for Raspberry Pi Pico (RP2040).

# IMPORTANT! Should *only be used for MIDI in*, i.e. from USB devices to hardware
# MIDI synths, and *not* in the opposite direction. Doing so could risk damage to
# the microcontroller, your computer, or both.

# Wire a 10Ω resistor from the Tx pin on the Raspberry Pi Pico or other 
# RP2040 board to the 3.5mm stereo jack tip, a 33Ω resistor from the 3v3 pin on
# the Raspberry Pi Pico to the 3.5mm stereo jack ring, and the ground pin on the
# Raspberry Pi Pico to the 3.5mm stereo jack sleeve.

# The code below assumes you're using USB MIDI channel 1. Change the number in
# square brackets on the midi_in line if you want to use a different channel.

import usb_midi
import board
import busio
import supervisor


import digitalio
led = digitalio.DigitalInOut(board.LED)
led.direction = digitalio.Direction.OUTPUT

toggle = True

def blink():
    global toggle
    led.value = toggle = not toggle

usb = usb_midi.ports[0]
usbOut = usb_midi.ports[1]
uart = busio.UART(tx=board.GP0, rx=board.GP1, baudrate=31250, timeout=0)

while True:
    # usb read, write to uart
    msg = usb.read(3)
    if len(msg):
        blink()
        uart.write(msg)
    # uart read, write to usb
    msg = uart.read()
    if msg is not None:
        blink()
        usbOut.write(msg)

