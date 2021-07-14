#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""wom_percentage_calculator.py:
    calculates the opening percentage after computing a perspective transformation
"""
__author__ = "Enrique Albertos"

import cv2
import numpy as np

class PercentageCalculator:    
    
    def __orderPoints(self, pts):
        # initialzie a list of coordinates that will be ordered
        # such that the first entry in the list is the top-left,
        # the second entry is the top-right, the third is the
        # bottom-right, and the fourth is the bottom-left
        rect = np.zeros((4, 2), dtype = "float32")
        # the top-left point will have the smallest sum, whereas
        # the bottom-right point will have the largest sum
        s = pts.sum(axis = 1)
        rect[0] = pts[np.argmin(s)]
        rect[2] = pts[np.argmax(s)]
        # now, compute the difference between the points, the
        # top-right point will have the smallest difference,
        # whereas the bottom-left will have the largest difference
        diff = np.diff(pts, axis = 1)
        rect[1] = pts[np.argmin(diff)]
        rect[3] = pts[np.argmax(diff)]
        # return the ordered coordinates
        return rect   
    
    def __getOpeningPercentage(self, trackerPoint, pts):
        try: 
            # obtain a consistent order of the points and unpack them
            # individually
            rect = self.__orderPoints(pts)
            (tl, tr, br, bl) = rect
            # compute the width of the new image, which will be the
            # maximum distance between bottom-right and bottom-left
            # x-coordiates or the top-right and top-left x-coordinates
            widthA = np.sqrt(((br[0] - bl[0]) ** 2) + ((br[1] - bl[1]) ** 2))
            widthB = np.sqrt(((tr[0] - tl[0]) ** 2) + ((tr[1] - tl[1]) ** 2))
            maxWidth = max(int(widthA), int(widthB))
            if int(maxWidth) == 0 :
                return 0
            # compute the height of the new image, which will be the
            # maximum distance between the top-right and bottom-right
            # y-coordinates or the top-left and bottom-left y-coordinates
            heightA = np.sqrt(((tr[0] - br[0]) ** 2) + ((tr[1] - br[1]) ** 2))
            heightB = np.sqrt(((tl[0] - bl[0]) ** 2) + ((tl[1] - bl[1]) ** 2))
            maxHeight = max(int(heightA), int(heightB))
            # the set of destination points to obtain a "birds eye view",
            dst = np.array([[0, 0], [maxWidth - 1, 0],
                [maxWidth - 1, maxHeight - 1], [0, maxHeight - 1]], dtype = "float32")
            # compute the perspective transform matrix
            M = cv2.getPerspectiveTransform(rect, dst)
            # transform the tracker point
            trackerTransform = np.matmul(M, np.array([[trackerPoint[0]],[trackerPoint[1]], [1]]))
            return int((trackerTransform[0] / trackerTransform[2]) / maxWidth * 100)
        except :
            return 0

        
    def calculate(self, mcorners) :
        # calculates the opening percentage given an ordered list of corners and a tracker
        # top left, top right, bottom right, bottom left, window tracker
        return self.__getOpeningPercentage(
            mcorners[4][0], # tracker point  
            np.array([
                mcorners[0][0], # ref. rec top left corner,
                mcorners[1][1], # ref. rec top right corner
                mcorners[2][2], # ref. rec bottom right corner
                mcorners[3][3]  # ref. rec bottom left corner
                ]))


if __name__ == '__main__':

    calculator = PercentageCalculator()
    corners = [np.array([[ 0, 0], [ 0, 0], [ 0, 0], [ 0, 0]], dtype="float32"),
               np.array([[ 100, 0], [ 100, 0], [ 100, 0], [ 100, 0]], dtype="float32"),
               np.array([[ 100, 100], [ 100, 100], [ 100, 100], [ 100, 100]], dtype="float32"),
               np.array([[ 0, 100], [ 0, 100], [ 0, 100], [ 0, 100]], dtype="float32"),
               np.array([[ 50, 0], [ 50, 0], [ 50, 0], [ 50, 0]], dtype="float32"),
               ]
    
    print("percentage ")
    print(calculator.calculate( corners ))
 


