#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""wom_window_detector_cloud.py: Window detector. Detects up to 4 windows
   marked with 5 ArUco markers each
   Results are sent to a 4x7 Led Display and to Ubidots Cloud
"""
__author__ = "Enrique Albertos"
__license__ = "GPL"

from imutils.video import VideoStream
import imutils
import time
import cv2
import numpy as np
from collections import deque
from _functools import reduce
from wom_display_4x7_spi  import Display4x7
from wom_ubidots_publisher import UbidotsPublisher
from wom_percentage_calculator import PercentageCalculator
import atexit

class WindowDetector() :
    __WINDOW1_MARKERS = ( 1,  2,  3,  4,  0)
    __WINDOW2_MARKERS = (11, 12, 13, 14, 10)
    __WINDOW3_MARKERS = (21, 22, 23, 24, 20)
    __WINDOW4_MARKERS = (31, 32, 33, 34, 30)
    __WINDOW_MARKERS = (__WINDOW1_MARKERS, __WINDOW2_MARKERS, __WINDOW3_MARKERS, __WINDOW4_MARKERS)
    __NO_MARKER_DETECTED = (False,False,False,False,False) 
    __NO_WINDOW_DETECTED = (__NO_MARKER_DETECTED,__NO_MARKER_DETECTED,__NO_MARKER_DETECTED,__NO_MARKER_DETECTED)
    __BUFFER_LENGTH = 40
    __FRAME_RATE = 4
    __IMAGE_SIZE = 1200
    __PUBLISH_REFRESH_TIME_S = 2
    

    lastViewedCorners = np.zeros((4,5,4,2))
    
    def __init__(self, display = Display4x7(), publisher = UbidotsPublisher()):
        self.__display = display
        self.__publisher = publisher
        self.__pecentageCalculator = PercentageCalculator()
        atexit.register(self.cleanup)
    
    def __movingDetector (self, iterable):
        # iterates the buffer deque and ors the lists of booleans
        return (reduce(lambda x, y: np.bitwise_or(list(x),list(y)), iterable)).tolist()
    
    def __markersInWindow(self, windowMarkers, ids) :
        # creates a tuple of booleans correspondig to the detection of the window markers
        # top left corner, top right corner, bottom right corner, left right corner, moving part
        list = []
        for element in windowMarkers :
            list.append(element in ids)
        return tuple(list)
    
    def __markersIn(self, windowMarkers, ids) :
        # creates a tuple of tuples for the different markers found in window 
        list = []
        for window in windowMarkers :
            list.append(self.__markersInWindow(window, ids))
        return tuple(list)
    
    def __extractCornersByWindow(self, windowMarkers, mcorners, flatid) :
        i = 0
        for windowMarker in windowMarkers :
            if windowMarker[0] in flatid :
                self.lastViewedCorners[i][0] = mcorners[int( np.where(flatid == windowMarker[0])[0])] # ref. rec top left corner,
            if windowMarker[1] in flatid :
                self.lastViewedCorners[i][1] = mcorners[int( np.where(flatid == windowMarker[1])[0])] # ref. rec top right corner
            if windowMarker[2] in flatid :
                self.lastViewedCorners[i][2] = mcorners[int( np.where(flatid == windowMarker[2])[0])] # ref. rec bottom right corner
            if windowMarker[3] in flatid :
                self.lastViewedCorners[i][3] = mcorners[int( np.where(flatid == windowMarker[3])[0])]  # ref. rec bottom left corner
            if windowMarker[4] in flatid :
                self.lastViewedCorners[i][4] = mcorners[int( np.where(flatid == windowMarker[4])[0])] # tracker point
            i += 1
    
    def __calculatePercentages(self) :
        percentages = []
        for i in range(4) :
              percentages.append( self.__pecentageCalculator.calculate(self.lastViewedCorners[i]))
        return percentages
    
    def start(self):
        lastUpdated = 0
        # starts the detector, grab images and display markers found
        detectorBuffer = deque((), maxlen= WindowDetector.__BUFFER_LENGTH)
        detectorBuffer.append(WindowDetector.__NO_WINDOW_DETECTED)

        self.__display.start() # start display thread
        arucoDict = cv2.aruco.Dictionary_get(cv2.aruco.DICT_4X4_50)
        arucoParams = cv2.aruco.DetectorParameters_create()

        vs = VideoStream(src=0, framerate=WindowDetector.__FRAME_RATE).start()
        # loop over the frames from the video stream
        while True:
            # grab the frame from the threaded video stream and resize it
            frame = vs.read()
            frame = imutils.resize(frame, width=WindowDetector.__IMAGE_SIZE)
            # detect ArUco markers in the input frame
            (mcorners, ids, rejected) = cv2.aruco.detectMarkers(frame, arucoDict, parameters=arucoParams)
            # verify *at least* one ArUco marker was detected
            if len(mcorners) > 0:
                flatid = ids.flatten();
                self.__extractCornersByWindow(self.__WINDOW_MARKERS, mcorners, flatid)
                if len(detectorBuffer) >= WindowDetector.__BUFFER_LENGTH:
                   detectorBuffer.popleft()                  
                detectorBuffer.append( self.__markersIn(self.__WINDOW_MARKERS, flatid))
            else:
                detectorBuffer.append(self.__NO_WINDOW_DETECTED)                
            self.__display.displayWindowCorners(self.__movingDetector(detectorBuffer))
            if time.time() - lastUpdated > self.__PUBLISH_REFRESH_TIME_S :
                self.__publisher.publish(self.__calculatePercentages())
                lastUpdated = time.time()
            
    def __enter__(self) :
        return self
    
    def __exit__(self, exc_type, exc_value, traceback) :
        self.cleanUp()
        
        
    def cleanup() :
        GPIO.cleanup()
        cv2.destroyAllWindows()
        vs.stop()

if __name__ == '__main__':
    try:
        windowDetector = WindowDetector()
        windowDetector.start()
    except KeyboardInterrupt:
       print('interrupted!')
        # do a bit of cleanup  
    sys.exit()

