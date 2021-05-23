############################################################################
#  This is an example for testing the VenTTracker Window Sensor
#  The sensor measures the opening of the window using only the accelerometer
#  When the device detects a complete movement it sends a stream of data
#  with the following structure. 
#  This scrtp parse the data a graph it.
#
#  START,accelSampleRate,accelRange,accelBandWidth, lpfSamples<LF>
#  accData,velData,posData,timestamp<LF>
#  accData,velData,posData,timestamp<LF>
#  ...
#  ...
#  ...
#  accData,velData,posData,timestamp<LF>
#  END<LF>
#  
#
#  Author: Enrique Albertos
#  Date: 2021-04-23
############################################################################




import serial
import time
import pandas as pd
import matplotlib.pyplot as plt
# if using a Jupyter notebook include
%matplotlib inline
# set up the serial line
ser = serial.Serial('COM17', 9600)
time.sleep(2)
while True:
    # Read and record the data
    accData =[]                       # empty list to store the data
    velData =[]                       # empty list to store the data
    posData =[]                       # empty list to store the data
    accelSampleRate = ""
    accelRange = ""
    accelBandWidth =""
    lpfSamples = ""
    timeStampData = []
    readings = 0
    while True:
        b = ser.readline()         # read a byte string
        string_n = b.decode()  # decode byte string into Unicode  
        string = string_n.rstrip() # remove \n and \r
        datalist = string.split(",")
        if (datalist[0] == "START"):
            accelSampleRate = datalist[1]
            accelRange = datalist[2]
            accelBandWidth = datalist[3]
            lpfSamples = datalist[4]
            break
    csvData = []
    while True:
        b = ser.readline()         # read a byte string
        csvData.append(b)
        string_n = b.decode()  # decode byte string into Unicode  
        string = string_n.rstrip() # remove \n and \r
        if (string == "END"):
            break
        readings = readings + 1
        datalist = string.split(",")
        accData.append(float(datalist[0]))           # add to the end of data list
        velData.append(float(datalist[1]))           # add to the end of data list
        posData.append(float(datalist[2]))           # add to the end of data list
        timeStampData.append(int(datalist[3]))           # add to the end of data list
    print(readings)
    if readings > 5:
        df = pd.DataFrame(csvData)
        df.to_csv("C:/Arduino/" + str(time.time()) + ".log")
        fig, axs = plt.subplots(1, 3, figsize=(14, 6), sharey=False)
        axs[0].plot(timeStampData, accData, label="Acceleration")
        axs[0].set_title('Acceleration')
        axs[0].set_xlabel('Time (ms)')            
        axs[0].set_ylabel('Acceleration (m/s^2)')
        axs[0].grid( True, color='0.95')
        axs[1].plot(timeStampData,velData, label="Velocity")
        axs[1].set_title('Velocity')
        axs[1].set_xlabel('Time (ms)')
        axs[1].set_ylabel('Velocity (m/s)')
        axs[1].grid( True, color='0.95')
        axs[2].plot(timeStampData,posData, label= "Distance") 
        axs[2].set_xlabel('Time (ms)')
        axs[2].set_ylabel('Distance (m)')
        axs[2].set_title('Distance')
        axs[2].grid(True, color='0.95')
        fig.suptitle('Distance. Sample Rate = ' + accelSampleRate + " Hz, Range +-" + accelRange + " G, Bandwidth " + accelBandWidth + " Hz" )
        plt.show()
