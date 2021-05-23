/* 
  This is an example for testing the VenTTracker Environmental Sensor
  connected to Arduino IoT Cloud.
  Displays data in a ST7735 LCD Display
  Uses BME280 /CCS811 Combo


  Cloud Variables:
      int cO2_Concentration_ppm;
      float pressure_bar;
      CloudRelativeHumidity relative_humidity;
      CloudTemperatureSensor temperature_combo_celsius;
      CloudTemperatureSensor temperature_imu_celsius;


  Author: Enrique Albertos
  Date: 2021-05-09




*/


#include "thingProperties.h"




#include <SPI.h>
#include <Wire.h>


#include <SparkFunBME280.h> //Click here to get the library: http://librarymanager/All#SparkFun_BME280
#include <SparkFunCCS811.h> //Click here to get the library: http://librarymanager/All#SparkFun_CCS811
#include <SparkFunLSM6DS3.h>


#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735


#include "DSEG14_Classic_Regular_20.h" 
#include "DSEG14_Classic_Regular_32.h" 


#define CCS811_ADDR 0x5B //Default I2C Address
//#define CCS811_ADDR 0x5A //Alternate I2C Address


#define PIN_NOT_WAKE 5


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 160 // OLED display height, in pixels
#define TFT_CS        10
#define TFT_RST        -1 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8




Adafruit_ST7735 display(TFT_CS, TFT_DC, TFT_RST);
LSM6DS3 IMU(I2C_MODE, 0x6A);


//Global sensor objects
CCS811 myCCS811(CCS811_ADDR);
BME280 myBME280;


bool envEnabled = false;


#define READINGS_INTERVAL 5000
unsigned long lastReadingAt;


void setup()
{
 Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(5000);


  initProperties();
  
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);


  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, onIoTConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, onIoTDisconnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, onIoTSync);


  setDebugMessageLevel(2);
  delay(3000);
  ArduinoCloud.printDebugInfo();
  lastReadingAt = millis();
}




//---------------------------------------------------------------
void loop()
{


  unsigned long msNow = millis();
  ArduinoCloud.update();
  //Check to see if data is available
    if (envEnabled) {
      if ( (msNow - lastReadingAt > READINGS_INTERVAL) && myCCS811.dataAvailable())
      {
        //Calling this function updates the global tVOC and eCO2 variables
        myCCS811.readAlgorithmResults();
        //printInfoSerial fetches the values of tVOC and eCO2
        //printInfoSerial();
        displayInfo();      
        infoToCloud();
        float BMEtempC = myBME280.readTempC();
        float BMEhumid = myBME280.readFloatHumidity();


        Serial.print("Applying new values (deg C, %): ");
        Serial.print(BMEtempC);
        Serial.print(",");
        Serial.println(BMEhumid);
        Serial.println();


        //This sends the temperature data to the CCS811
        myCCS811.setEnvironmentalData(BMEhumid, BMEtempC);
        lastReadingAt = msNow;


      }
      else if (myCCS811.checkForStatusError())
      {
        //If the CCS811 found an internal error, print it.
        printSensorError();
          CCS811Core::CCS811_Status_e returnCode = myCCS811.beginWithStatus();
          Serial.print("CCS811 begin exited with: ");
          Serial.println(myCCS811.statusString(returnCode));
      }
  }


}


void setupIMU() {
  IMU.begin();
}


void setupEnvironmentalCombo() {
  Serial.println("Apply BME280 data to CCS811 for compensation.");


  Wire.begin();
  display.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab


  //This begins the CCS811 sensor and prints error status of .beginWithStatus()
  CCS811Core::CCS811_Status_e returnCode = myCCS811.beginWithStatus();
  Serial.print("CCS811 begin exited with: ");
  Serial.println(myCCS811.statusString(returnCode));


  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;


  //Initialize BME280
  //For I2C, enable the following and disable the SPI section
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;
  myBME280.settings.runMode = 3; //Normal mode
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;


  //Calling .begin() causes the settings to be loaded
  delay(10); //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  myBME280.begin();


}






//---------------------------------------------------------------
void printInfoSerial()
{
  //getCO2() gets the previously read data from the library
  Serial.println("CCS811 data:");
  Serial.print(" CO2 concentration : ");
  Serial.print(myCCS811.getCO2());
  Serial.println(" ppm");


  //getTVOC() gets the previously read data from the library
  Serial.print(" TVOC concentration : ");
  Serial.print(myCCS811.getTVOC());
  Serial.println(" ppb");


  Serial.println("BME280 data:");
  Serial.print(" Temperature: ");
  Serial.print(myBME280.readTempC(), 2);
  Serial.println(" degrees C");


  Serial.print(" Temperature: ");
  Serial.print(myBME280.readTempF(), 2);
  Serial.println(" degrees F");


  Serial.print(" Pressure: ");
  Serial.print(myBME280.readFloatPressure()*100, 2);
  Serial.println(" hPa");


  Serial.print(" Pressure: ");
  Serial.print((myBME280.readFloatPressure() * 0.0002953), 2);
  Serial.println(" InHg");


  Serial.print(" Altitude: ");
  Serial.print(myBME280.readFloatAltitudeMeters(), 2);
  Serial.println("m");


  Serial.print(" Altitude: ");
  Serial.print(myBME280.readFloatAltitudeFeet(), 2);
  Serial.println("ft");


  Serial.print(" %RH: ");
  Serial.print(myBME280.readFloatHumidity(), 2);
  Serial.println(" %");


  Serial.println();
}




void infoToCloud()
{
  cO2_Concentration_ppm = myCCS811.getCO2();
  temperature_combo_celsius =((int)(myBME280.readTempC() *10) )/ 10.0;
  pressure_bar =(int) (myBME280.readFloatPressure()/100); //hpa
  relative_humidity = myBME280.readFloatHumidity();
  temperature_imu_celsius = ((int)(IMU.readTempC()*10)) / 10.0;
}




void displayInfo()
{
  clearDisplay();
  display.setTextColor(ST7735_BLACK);
  
  display.setCursor(0, 50 );
  display.setFont(&DSEG7_Classic_Regular_32);
  display.setTextSize(0);
  display.print(myCCS811.getCO2());
  display.setTextSize(0);
  display.setFont(NULL);
  display.print(" ppm");


  display.setCursor(0, 85 );
  display.setFont(&DSEG14_Classic_Regular_20);
  display.print(myBME280.readTempC(), 1);
  display.setFont(NULL);
  display.print(" C");


  display.setCursor(0, 115 );
  display.setFont(&DSEG14_Classic_Regular_20);
  display.print(myBME280.readFloatPressure()/100.0, 0);
  display.setFont(NULL);
  display.print(" HPa");


  display.setCursor(0, 145 );
  display.setFont(&DSEG14_Classic_Regular_20);
  display.print(myBME280.readFloatHumidity(), 1);
  display.setFont(NULL);
  display.print(" %");


}


//printSensorError gets, clears, then prints the errors
//saved within the error register.
void printSensorError()
{
  uint8_t error = myCCS811.getErrorRegister();


  if (error == 0xFF) //comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  }
  else
  {
    Serial.print("Error: ");
    if (error & 1 << 5)
      Serial.print("HeaterSupply");
    if (error & 1 << 4)
      Serial.print("HeaterFault");
    if (error & 1 << 3)
      Serial.print("MaxResistance");
    if (error & 1 << 2)
      Serial.print("MeasModeInvalid");
    if (error & 1 << 1)
      Serial.print("ReadRegInvalid");
    if (error & 1 << 0)
      Serial.print("MsgInvalid");
    Serial.println();
  }
}


uint16_t getBackgroundColor(){
  return ST7735_WHITE;
}


void clearDisplay() {
 display.fillScreen(getBackgroundColor());
}


void onIoTConnect() {
  // enable your other i2c devices
  Serial.println(F(">>> connected to Arduino IoT Cloud"));
  Serial.println(F("enabling other i2c devices"));
  // run the setup code (begin or else) for your other i2c devices
  display.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  clearDisplay();
  Serial.println(F("ST77xx TFT Begin"));
  setupIMU(); 
  Serial.println(F("IMU Begin"));
  setupEnvironmentalCombo();
  Serial.println(F("I2C Environmental COMBO Begin"));
}


void onIoTDisconnect() {
  // disable your other i2c devices
  Serial.println(F(">>> disconnected to Arduino IoT Cloud"));
  Serial.println(F("disabling other i2c devices"));
  // if necessary call the end() method on your i2c device library or even get rid of them.
  // really it's on a peripheral basis
  envEnabled = false;
}


void onIoTSync() {
  Serial.println(">>> Board and Cloud SYNC OK");  
}
