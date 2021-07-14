#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""wom_ubidots_publisher.py: Sends location and Windows state to Ubidots Cloud
"""
__author__ = "Enrique Albertos"

# Venttracker WOM Windows Opening Monitor creation on Ubidots
import time
import requests
import json
import random

class UbidotsCredentials() :
    TOKEN = "XXXX"  # Put your TOKEN here
    DEVICE_LABEL = "XXXX"  # Put your device label here
    
    def __init__(self, token = TOKEN, deviceLabel = DEVICE_LABEL):
        self.__token = token
        self.__deviceLabel = deviceLabel
        
    def getToken(self) :
        return self.__token
    
    def getDeviceLabel(self) :
        return self.__deviceLabel

class UbidotsPublisher() :
    
    __WINDOWS_NUMBER_LABEL = "windows_no"  # Number of windows detected
    __UBIDOTS_URL = "http://industrial.api.ubidots.com"
    __UBIDOTS_SERVICE_ADDRESS = "{}/api/v1.6/devices/{}"
    __GEOLOCATE_URL = 'https://extreme-ip-lookup.com/json/'
    __HTTP_400_BAD_REQUEST = 400
    __POSITION_LABEL = "position"  # Number of windows detected
    __WOM_SERIAL_LABEL = "serial_id"
    __W01_OPENING_PCT_LABEL = "w01_opening_pct"  # % opening window 1
    __W02_OPENING_PCT_LABEL = "w02_opening_pct" # % opening window 2
    __W03_OPENING_PCT_LABEL = "w03_opening_pct"  # % opening window 3
    __W04_OPENING_PCT_LABEL = "w04_opening_pct"  # % opening window 4
    __W05_OPENING_PCT_LABEL = "w05_opening_pct"  # % opening window 5
    __INFO_ATTEMPING_TO_SEND_DATA = "[INFO] Attemping to send data."
    __INFO_FINISHED = "[INFO] finished."
    __INFO_PAYLOAD = "[INFO] {}."
    __INFO_REQUEST_UPDATED = "[INFO] request made properly, your device is updated."
    __ERROR_FIVE_ATTEMPTS = "[ERROR] Could not send data after 5 attempts, please check your token credentials and internet connection."
    __REQUEST_ATTEMPTS = 5
    __WINDOWS_NUMBER = 4
    __WINDOWS_LABELS = [__W01_OPENING_PCT_LABEL, __W02_OPENING_PCT_LABEL, \
                      __W03_OPENING_PCT_LABEL, __W04_OPENING_PCT_LABEL]
    
    def __init__(self, credentials = UbidotsCredentials(), debug = False):
        self.__credentials = credentials
        self.__location = self.__geolocate()    
        self.__serial = self.__get_serial()
        self.__debug = debug # True activate debug print
        

    # get latitude and longitude from IP
    def __geolocate(self) :
        url = UbidotsPublisher.__GEOLOCATE_URL
        r = requests.get(url)
        data = json.loads(r.content.decode())
        return {'lat' : data['lat'],'lng': data['lon']}

    # get raspberry pi seriial as unique identifier
    def __get_serial(self):
        # Extract serial from cpuinfo file
        cpuserial = "0000000000000000"
        try:
            f = open('/proc/cpuinfo','r')
            for line in f:
                if line[0:6]=='Serial':
                    cpuserial = line[10:26]
            f.close()
        except:
            cpuserial = "ERROR000000000"
        return cpuserial

    # build payload dictionary
    def __build_payload(self, windowData, location, serial):
        
        payload = {UbidotsPublisher.__POSITION_LABEL:
             {"value": "1",
              "context": {"lat": location['lat'],
                          "lng": location['lng'],
                          UbidotsPublisher.__WOM_SERIAL_LABEL : serial }},        
              UbidotsPublisher.__WINDOWS_NUMBER_LABEL: UbidotsPublisher.__WINDOWS_NUMBER}  
        
        for i in range(0, UbidotsPublisher.__WINDOWS_NUMBER): 
            payload.update({self.__WINDOWS_LABELS[i]: windowData[i]})
            
        payload.update(
            {UbidotsPublisher.__POSITION_LABEL:
             {"value": "1",
              "context": {"lat": location['lat'],
                          "lng": location['lng'],
                          UbidotsPublisher.__WOM_SERIAL_LABEL : serial }}})

        if self.__debug:
            print(UbidotsPublisher.__INFO_PAYLOAD.format(payload))

        return payload

    # post request to Ubidots
    def __post_request(self, payload):
        # Creates the headers for the HTTP requests
        url = UbidotsPublisher.__UBIDOTS_URL
        url = UbidotsPublisher.__UBIDOTS_SERVICE_ADDRESS.format(url, self.__credentials.getDeviceLabel())
        headers = {"X-Auth-Token": self.__credentials.getToken(), "Content-Type": "application/json"}

        # Makes the HTTP requests
        status = UbidotsPublisher.__HTTP_400_BAD_REQUEST
        attempts = 0
        while status >= UbidotsPublisher.__HTTP_400_BAD_REQUEST and attempts <= UbidotsPublisher.__REQUEST_ATTEMPTS:
            req = requests.post(url=url, headers=headers, json=payload)
            status = req.status_code
            attempts += 1
            time.sleep(1)

        # Processes results
        if self.__debug :
            print(req.status_code, req.json())
        if status >= UbidotsPublisher.__HTTP_400_BAD_REQUEST:
            if self.__debug :
                print(UbidotsPublisher.__ERROR_FIVE_ATTEMPTS)
            return False

        if self.__debug :
            print(UbidotsPublisher.__INFO_REQUEST_UPDATED)
        return True

    def publish(self, windowData) :
        payload = self.__build_payload(windowData, self.__location, self.__serial)

        if self.__debug :
            print(UbidotsPublisher.__INFO_ATTEMPING_TO_SEND_DATA)
        self.__post_request(payload)
        if self.__debug :
            print(UbidotsPublisher.__INFO_FINISHED)
        


if __name__ == '__main__':    
    cloudPublisher = UbidotsPublisher(debug=True)
    cloudPublisher.publish([random.randint(0, 100) for _ in range(5)])
 