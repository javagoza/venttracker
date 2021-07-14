#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""wom_display_4x7_spi.py: 4 digits x7  segment display
   drives a 4x7 segment display using an SN74HC595 shif register clocked by spi clock
   and 4 digital lines to switch didits. Works in its own thread
"""
__author__ = "Enrique Albertos"
__license__ = "GPL"

import RPi.GPIO as GPIO
import sys
import time
import threading
from threading import Thread
import spidev
import atexit

class Display4x7(threading.Thread):
    
    # PIN definitions GPIO.BCM
    # Connect to 74HC595 8-bit serial-in, parallel-out shift 
    __bus  = 0  # MOSI GPIO 10 (PIN 21) - 74HC595 pin 14 DS
              # SCLK GPIO 11 - 74HC595 pin 11 SHCP
    __device = 0
    __spiSpeedDefault = 3900000
    __latchPinDefault = 25 # GPIO 8 (CEO) 74HC595 pin 12 STCP


    # HS42056 1K-32 digit selection
    __digit0PinDefault   = 14 # 7-Segment pin D4
    __digit1PinDefault   = 15 # 7-Segment pin D3
    __digit2PinDefault   = 18 # 7-Segment pin D2
    __digit3PinDefault   = 23 # 7-Segment pin D1

    MARKERS = ( 0x03,  # Top Left
                0x05,  # Top Right
                0x50,  # Bottom Right
                0x18,  # Bottom Left
                0x80,  # Center
                0x00   # blank
                )

    HEX_DIGITS = (0x5F, # = 0  
                 0x44,  # = 1  
                 0x9D,  # = 2  
                 0xD5,  # = 3
                 0xC6,  # = 4
                 0xD3,  # = 5
                 0xDB,  # = 6
                 0x45,  # = 7
                 0xDF,  # = 8
                 0xC7,  # = 9
                 0xCF,  # = A
                 0xDA,  # = b
                 0x1B,  # = C
                 0xDC,  # = d
                 0x9B,  # = E
                 0x8B,  # = F
                 0x00   # blank
                 )    
    
    def __init__(self, initialContent = (0,0,0,0), bus=0, device=0, digit0 = __digit0PinDefault, digit1 = __digit1PinDefault, digit2 = __digit2PinDefault, digit3 = __digit3PinDefault,  latchPin = __latchPinDefault, speedHz = __spiSpeedDefault):
        self.__displayContent = initialContent
        self.__latchPin = latchPin
        self.__digit3 = digit3
        self.__digit2 = digit2
        self.__digit1 = digit1
        self.__digit0 = digit0
        self.__shifRegisterPins = (latchPin)
        self.__controlDigitsPins = ( digit3, digit2, digit1, digit0 )
        self.__lock = threading.Lock()
        self.__bus = bus
        self.__device = device
        self.__speedHz = speedHz
        atexit.register(self.cleanup)
        self.__setup()
        threading.Thread.__init__(self)          
    
    def __initPinsAsOutputs(self, pins) :
        for pin in pins:
            GPIO.setup(pin, GPIO.OUT, initial = GPIO.LOW)
            
    def __lowPins(self, pins) :
        for pin in pins:
            GPIO.output(pin, GPIO.LOW)           
            
    def __setup(self):        
        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM)
        # init display control digits pins
        self.__initPinsAsOutputs(self.__controlDigitsPins)        
        # init serial shit register pins
        GPIO.setup(self.__latchPin, GPIO.OUT, initial = GPIO.LOW)
        
        self.__spiDisplay= spidev.SpiDev()
        self.__spiDisplay.open(self.__bus,self.__device)
        self.__spiDisplay.max_speed_hz = self.__speedHz
        self.__spiDisplay.mode = 0
        self.__spiDisplay.bits_per_word = 8
        self.__spiDisplay.no_cs = True        
        
    def __shiftout(self, byte):        
        GPIO.output(self.__latchPin, 1)
        time.sleep(0.00000005)
        GPIO.output(self.__latchPin, 0)
        self.__spiDisplay.xfer([byte])
        GPIO.output(self.__latchPin, 1)
        time.sleep(0.00000005)
        GPIO.output(self.__latchPin, 0)
        
    def run(self):
        # overrides thread run
        while True:
            i=0
            for pin in self.__controlDigitsPins:
                self.__lowPins(self.__controlDigitsPins)    
                with self.__lock:
                    self.__shiftout(self.__displayContent[i])
                GPIO.output(pin, GPIO.HIGH)
                time.sleep(0.00000001)
                i=i+1
               
    def display(self, displayContent = (0,0,0,0)) :
        with self.__lock:
            self.__displayContent = displayContent
    
    def displayInt(self, number = 0) :
        self.display((self.HEX_DIGITS[(number // 1000)%10], self.HEX_DIGITS[(number // 100)%10],self.HEX_DIGITS[(number // 10)%10],self.HEX_DIGITS[number %10]))

    def displayWindowCorners(self, iterable) :
        content = [0,0,0,0]
        digit=0
        for element in iterable:
            for i in range(5) :
                if element[i]:
                    content[digit] |= Display4x7.MARKERS[i]
            digit = digit + 1
        self.display(content)
    
    def __enter__(self) :
        return self
    
    def __exit__(self, exc_type, exc_value, traceback) :
        self.cleanUp()
        
        
    def cleanup() :
        self.__dislay.closeSPI(self.spiDevice)
        GPIO.cleanup()
    

if __name__ == '__main__':
    try:

        display = Display4x7()
        display.start()
        i=0
        while True :
            display.displayWindowCorners([[True, True, True, True, True], [False, False, False, False, False], [False, False, False, False, False], [False, False, False, False, False]])
            #display.display((Display4x7.HEX_DIGITS[i%16],Display4x7.HEX_DIGITS[(i+1)%16],Display4x7.HEX_DIGITS[(i+2)%16],Display4x7.HEX_DIGITS[(i+3)%16]))
            i = i + 1
            time.sleep(.5)
    except KeyboardInterrupt:

       print('interrupted!')
       GPIO.cleanup()
    sys.exit()

        



