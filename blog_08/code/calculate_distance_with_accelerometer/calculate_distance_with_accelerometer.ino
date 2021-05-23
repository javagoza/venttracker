/**************************************************************************
  This is an example for testing the VenTTracker Window Sensor
  The sensor measures the opening of the window using only the accelerometer
  The measurement is carried out by making a double integration of the acceleration values. 
  
  This example is for a 128x32 pixel display using I2C to communicate
  3 pins are required to interface (two I2C and one reset).


  Hardware. Pinout
    8 A4/SDA Analog ADC in; I2C SDA; SDA to SDA OLED Display
    9 A5/SCL Analog ADC in; I2C SCL to SCK OLED Display
    13 RST Digital In Active low reset input  RESET to push button. Other end to ground.
    14 GND Power Power Ground to Battery (-)
    15 VIN Power In Vin Power input VIN to Baterry (+)
    20 D2 Digital GPIO - to Open/Closed Reed Switches
    27 D9/PWM Digital GPIO to  Right Reed Switch. Other end to ground.
    28 D10/PWM Digital GPIO;  to Left Reed Switch. Other end to ground.


  Author: Enrique Albertos
  Date: 2021-04-23
 **************************************************************************/
#include "SparkFunLSM6DS3.h"


#include "SPI.h"
#include "Wire.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBold18pt7b.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels


#define G_CONSTANT  9.80665 // m/s^2 adjust to your location


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


long lastMillis = millis();


#define DISRIMINATION_WINDOW_X 0.019
#define DISRIMINATION_WINDOW_Y 0.019
#define DISRIMINATION_WINDOW_Z 0.1
#define LPF_SAMPLES 16.0  //16


LSM6DS3 IMU(I2C_MODE, 0x6A);


uint16_t errorsAndWarnings = 0;
float maximumX = 0;
// last accelerometer reading
float lastAccXsample;
float lastAccYsample;
float lastAccZsample;
// buffer for logging
float accSamples[1000];
float velSamples[1000];
float posSamples[1000];
long timestampSamples[1000];
int rawSamplesCount = 0;
// last two readings for integration
unsigned char countx;
float accelerationX[2];
float velocityX[2];
float positionX[2];
float accelerationY[2];
float velocityY[2];
float positionY[2];
float accelerationZ[2];
float velocityZ[2];
float positionZ[2];
unsigned char direction;
float stationaryReadingX;
float stationaryReadingY;
float stationaryReadingZ;
// let's assume we are in closed position and window opens from left to right
volatile bool openCloseChangePending = true;
unsigned long lastLeftSwitchDebounceTime = 0; // the last time the input left encoder pin was toggled
unsigned long OpenClosedSwitchDebounceDelay = 150; // the debounce time
unsigned long lastDebouncingMeterMillis= 0; // last time the meter is stopped
unsigned long accelerometerDebounceDelay = 300; // the debounce time
enum WindowStateType {OPEN = HIGH, CLOSED = LOW};
int windowState = CLOSED;
bool directionChanged = false;
int openClosedSwitchPort = 2; // switch to ground + internal pull-up resistor.
                              // Negative logic LOW when switch is closed


void calibrateAccelerometer(void);
void movementEndCheck(void);
void position(void);
void resetPosition(void);
void displayPosition(void);
void isrChangeOpenClosedSwitchPort() ;


void setup() {
  setupSerialPort();
  setupAccelerometer();
  setupDisplay();
  calibrateAccelerometer();
  setUpOpenCLoseSwitch();
}


void loop() {
  int16_t temp;
  windowState = digitalRead(openClosedSwitchPort);
  if(windowState == OPEN && openCloseChangePending ) {
    openCloseChangePending = false;
    if (rawSamplesCount > 3) {
      logDataToSerial();
    }
    rawSamplesCount = 0;
  }
  if(windowState == CLOSED || openCloseChangePending ) {
    openCloseChangePending = false;
    rawSamplesCount = 0;
    resetPosition();
    displayPosition();
  }  else {
    position();
  }
}


void setupSerialPort() {
  Serial.begin(9600);
  delay(1000); 
}


void setupDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    for (;;)
      ; // Don't proceed, loop forever
  }
}


void setUpOpenCLoseSwitch() {
  pinMode(openClosedSwitchPort, INPUT_PULLUP);
  delay(20);
  // detect switch chages from open to closed. It is negative logic
  attachInterrupt(digitalPinToInterrupt(openClosedSwitchPort),
                  isrChangeOpenClosedSwitchPort, CHANGE);
}


void setupAccelerometer() {
  IMU.settings.gyroEnabled = 0; 
  IMU.settings.accelEnabled = 1;
  IMU.settings.accelRange = 2;      //Max G force readable.  Can be: 2, 4, 8, 16
  IMU.settings.accelSampleRate = 416;  //Hz.  Can be: 13, 26, 52, 104, 208, 416, 833, 1666, 3332, 6664, 13330
  IMU.settings.accelBandWidth = 100;  //Hz.  Can be: 50, 100, 200, 400;
  IMU.settings.accelFifoEnabled = 0;  //Set to include accelerometer in the FIFO
  IMU.settings.tempEnabled = 0;
  IMU.settings.fifoModeWord = 0; 
  if (!IMU.begin() ) {
    Serial.print("Error at begin().\n");
  }
}


void displayPosition(void) {
  display.setFont();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.print(accelerationX[1], 4);
  display.setCursor(0, 8);
  display.print(velocityX[1], 4);
  // show distance in cm
  float distance = sqrt(positionX[1]*positionX[1] + positionY[1] * positionY[1]+ positionZ[1] * positionZ[1]) * 100.0;
  display.setCursor(0, 16);
  display.print(distance, 4);
  display.setFont(&FreeMonoBold18pt7b);
  display.setCursor(64, 24);
  display.print((int)distance);
  display.setFont();
  display.print("cm");
  display.display();
}


/**
  Interrupt Service Routine for Changing Edge in Open Close Switches
*/
void isrChangeOpenClosedSwitchPort() {
  windowState = digitalRead(openClosedSwitchPort);
  if ((millis() - lastLeftSwitchDebounceTime) > OpenClosedSwitchDebounceDelay) {
    openCloseChangePending = true;
    lastLeftSwitchDebounceTime = millis();
  }
}


void resetPosition(void) {
  accelerationX[1] = 0;
  accelerationX[0] = 0;
  velocityX[1] = 0;
  velocityX[0] = 0;
  positionX[0] = 0;
  positionX[1] = 0;


  accelerationY[1] = 0;
  accelerationY[0] = 0;
  velocityY[1] = 0;
  velocityY[0] = 0;
  positionY[1] = 0;
  positionY[0] = 0;


  accelerationZ[1] = 0;
  accelerationZ[0] = 0;
  velocityZ[1] = 0;
  velocityZ[0] = 0;
  positionZ[1] = 0;
  positionZ[0] = 0;


  lastMillis = millis();
}


// read accelerometer data
void readAccelerometerData() {
  lastAccXsample = -IMU.readFloatAccelX();
  lastAccYsample = IMU.readFloatAccelY();
  lastAccZsample = IMU.readFloatAccelZ();
}


void displayCalibrationInfo() {
  display.clearDisplay();
  display.setCursor(10,10);
  display.fillScreen(BLACK);
  display.setTextColor(WHITE);
  display.println(F("Calibrating IMU..."));
  display.print(F("Sample rate: "));
  display.print(IMU.settings.accelSampleRate);
  display.print(F(" Hz"));
  display.display();
  delay(20);
}


/*******************************************************************************
  The purpose of the calibration routine is to obtain the value of the reference
threshold. It consists on a 1024 samples average in no-movement condition.
********************************************************************************/
void calibrateAccelerometer(void) {
  unsigned int count1 =0;
  displayCalibrationInfo();
  stationaryReadingX = 0.0;
  stationaryReadingY = 0.0;
  stationaryReadingZ = 0.0;
  // calculate mean stationary value
  do {
      readAccelerometerData();
      stationaryReadingX = stationaryReadingX + lastAccXsample; // Accumulate Samples
      stationaryReadingY = stationaryReadingY + lastAccYsample; 
      stationaryReadingZ = stationaryReadingZ + lastAccZsample; 
      count1++;    
  } while (count1 != 1024); // 1024 times
  stationaryReadingX = stationaryReadingX / 1024.0;
  stationaryReadingY = stationaryReadingY / 1024.0;
  stationaryReadingZ = stationaryReadingZ / 1024.0; 
}


/*
Send data buffers to serial
*/
void logDataToSerial() {
  Serial.print("START");
  Serial.print(",");
  Serial.print(IMU.settings.accelSampleRate);
  Serial.print(",");
  Serial.print(IMU.settings.accelRange);
  Serial.print(",");
  Serial.print(IMU.settings.accelBandWidth);
  Serial.print(",");
  Serial.println(LPF_SAMPLES);


  for (int i = 0; i < rawSamplesCount; ++i) {
    Serial.print(accSamples[i], 8);
    Serial.print(",");
    Serial.print(velSamples[i], 8);
    Serial.print(",");
    Serial.print(posSamples[i], 8);
    Serial.print(",");
    Serial.print(timestampSamples[i]-timestampSamples[0]);
    Serial.println();
  }
  Serial.println("END");
}


/*
  This function allows movement end detection. If a certain number of
acceleration samples are equal to zero or velocity direction has changed the sign 
we can assume movement has stopped.
Velocity variables are reseted, this stops position increment and eliminates position error.
*/
void movementEndCheck(void) {
  
if ( accelerationX[1] == 0 )// we can assume that velocity is cero
 // we count the number of acceleration samples that equals cero
  {
    countx++;
  } else {
    countx = 0;
  }


if (countx > 2 || directionChanged ) 
   {
    displayPosition();
   
    countx = 0;
    resetAccelerationBuffer();
    resetVelocityBuffer();
    directionChanged = false;


    if (rawSamplesCount > 4) {
      logDataToSerial();
    }
    rawSamplesCount = 0;
    lastDebouncingMeterMillis = millis();
   }
  displayPosition();
}


void resetAccelerationBuffer() {
    accelerationX[0] = 0.0;
    accelerationX[1] = 0.0;
    accelerationY[0] = 0.0;
    accelerationY[1] = 0.0;
    accelerationZ[0] = 0.0;
    accelerationZ[1] = 0.0;
}
void resetVelocityBuffer() {
    velocityX[0] = 0.0;
    velocityX[1] = 0.0;
    velocityY[0] = 0.0;
    velocityY[1] = 0.0;
    velocityZ[0] = 0.0;
    velocityZ[1] = 0.0;
}


void debounceAccelerometer() {
  while (millis() < lastDebouncingMeterMillis + accelerometerDebounceDelay){
    readAccelerometerData();
    resetAccelerationBuffer();
    resetVelocityBuffer();
  }
}


/*****************************************************************************************/
/******************************************************************************************
  This function transforms acceleration to a proportional position by
integrating  the acceleration data  twice. It also adjusts sensibility by
multiplying the "positionX"  and "positionY" variables. This integration
algorithm carries error, which is compensated in the "movenemt_end_check"
  subroutine. Faster  sampling frequency implies less error  but requires more
memory. Keep  in mind that the same process is applied to the X and Y axis.
*****************************************************************************************/


void position(void) {

  unsigned int count2;
  count2 = 0;
  debounceAccelerometer(); 
  // filtering routine for noise attenuation average represents the acceleration of an instant
  do {    
    readAccelerometerData();
    accelerationX[1] = accelerationX[1] + lastAccXsample;
    accelerationY[1] = accelerationY[1] + lastAccYsample;
    accelerationZ[1] = accelerationZ[1] + lastAccZsample;
    count2++;


  } while (count2 != LPF_SAMPLES); // 64 sums of the acceleration sample


  // Low pass band filter
  accelerationX[1] = accelerationX[1] / LPF_SAMPLES; // division by 64
  accelerationX[1] = accelerationX[1] - stationaryReadingX; // eliminating zero reference
  
  accelerationY[1] = accelerationY[1] / LPF_SAMPLES; // division by 64
  accelerationY[1] = accelerationY[1] - stationaryReadingY; // eliminating zero reference


  accelerationZ[1] = accelerationZ[1] / LPF_SAMPLES; // division by 64
  accelerationZ[1] = accelerationZ[1] - stationaryReadingZ; // eliminating zero reference
  // offset of the acceleration data


  // mechanical filter
  if ((accelerationX[1] <= DISRIMINATION_WINDOW_X) &&
      (accelerationX[1] >=-DISRIMINATION_WINDOW_X)) // Discrimination window applied
  {
    accelerationX[1] = 0; // to the X axis acceleration
  }


  if ((accelerationY[1] <= DISRIMINATION_WINDOW_Y) &&
      (accelerationY[1] >=-DISRIMINATION_WINDOW_Y)) // Discrimination window applied
  {
    accelerationY[1] = 0; // to the Y axis acceleration
  }


  if ((accelerationZ[1] <= DISRIMINATION_WINDOW_Z) &&
      (accelerationZ[1] >=-DISRIMINATION_WINDOW_Z)) // Discrimination window applied
  {
    accelerationZ[1] = 0; // to the Z axis acceleration
  }


  // variable
  long actualTime = millis();
  long ellapsedTime = actualTime - lastMillis;
  lastMillis = actualTime;


  // first X integration:
  velocityX[1] = velocityX[0] + (accelerationX[0]  + ((accelerationX[1] - accelerationX[0]) / 2.0)) *  G_CONSTANT * ellapsedTime /1000.0 ;
  velocityY[1] = velocityY[0] + (accelerationY[0]  + ((accelerationY[1] - accelerationY[0]) / 2.0)) *  G_CONSTANT * ellapsedTime /1000.0;
  velocityZ[1] = velocityZ[0] + (accelerationZ[0]  + ((accelerationZ[1] - accelerationZ[0]) / 2.0)) *  G_CONSTANT * ellapsedTime /1000.0;


  // second X integration:
  if ( (velocityX[0] * velocityX[1]) > 0) {
    positionX[1] = positionX[0] + (velocityX[0] + ((velocityX[1] - velocityX[0]) / 2.0)) * ellapsedTime/1000.0;
    positionY[1] = positionY[0] + (velocityY[0] + ((velocityY[1] - velocityY[0]) / 2.0)) * ellapsedTime/1000.0;
    positionZ[1] = positionZ[0] + (velocityZ[0] + ((velocityZ[1] - velocityZ[0]) / 2.0)) * ellapsedTime/1000.0;
  }


  if (rawSamplesCount == 0 ) {
    accSamples[rawSamplesCount] = accelerationX[0];
    velSamples[rawSamplesCount] = velocityX[0];
    posSamples[rawSamplesCount] = positionX[0];
    timestampSamples[rawSamplesCount] = millis()-ellapsedTime;
    rawSamplesCount++;
  }
  
  accelerationX[0] =  accelerationX[1]; // The current acceleration value must be sent to the previous acceleration
  directionChanged = (velocityX[0] * velocityX[1]) < 0;
  velocityX[0] = velocityX[1]; // Same done for the velocity variable
  positionX[0] = positionX[1]; // actual position data must be sent to the


  accelerationY[0] =  accelerationY[1]; // The current acceleration value must be sent to the previous acceleration
  velocityY[0] = velocityY[1]; // Same done for the velocity variable
  positionY[0] = positionY[1]; // actual position data must be sent to the


  accelerationZ[0] =  accelerationZ[1]; // The current acceleration value must be sent to the previous acceleration
  velocityZ[0] = velocityZ[1]; // Same done for the velocity variable
  positionZ[0] = positionZ[1]; // actual position data must be sent to the




  accSamples[rawSamplesCount] = accelerationX[1];
  velSamples[rawSamplesCount] = velocityX[1];
  posSamples[rawSamplesCount] = positionX[1];
  timestampSamples[rawSamplesCount] = millis();
  rawSamplesCount++;


  movementEndCheck();


  direction = 0; // data variable to direction variable reset
}





