/* 
  This is an example for testing the VenTTracker Environmental Sensor
  connected to Arduino IoT Cloud.
  Displays data in a ST7735 LCD Display
  Uses CCS811 and HTU21 modules

  Cloud Variables:
      int cO2_Concentration_ppm;
      float pressure_bar;
      CloudRelativeHumidity relative_humidity;
      CloudTemperatureSensor temperature_combo_celsius;
      CloudTemperatureSensor temperature_imu_celsius;
  Author: Enrique Albertos
  Date: 2021-05-21
*/
//#define DEBUG
#include "thingProperties.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735

#include "fonts/FreeMonoBoldOblique18pt7b.h"
#include "fonts/FreeMonoBold9pt7b.h"

#include "SparkFunCCS811.h" //Click here to get the library: http://librarymanager/All#SparkFun_CCS811
#include "SparkFunHTU21D.h"
#include <SparkFunLSM6DS3.h>
extern RTCZero rtc;
const int GMT = 2; //change this to adapt it to your time zone

#define GAUGE_GREEN 0x0320
#define GAUGE_YELLOW 0xFFE0
#define GAUGE_ORANGE 0xFC60
#define GAUGE_RED 0xF800
#define ST77XX_GRAY_C8 0xCE59
#define ST77XX_GRAY_FA 0xFFDF

#define CCS811_ADDR 0x5A //Alternate I2C Address
CCS811 myCCS811(CCS811_ADDR);
LSM6DS3 IMU(I2C_MODE, 0x6A);
HTU21D myHumidity;

int status = WL_IDLE_STATUS;

//please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key index number (needed only for WEP)

long lastEnvironmentUpdate;
long lastTimeUpdate;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 160 // OLED display height, in pixels
#define TFT_CS        10
#define TFT_RST        -1 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

#define DEG2RAD 0.0174532925

// icons
const unsigned short termo6x16[96] PROGMEM={
0x0000, 0x31A6, 0xFFFF, 0xFFFF, 0x31A6, 0x0000, 0x0000, 0xFFFF, 0x31A6, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000,   // 0x0010 (16) pixels
0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x31A6, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF,   // 0x0020 (32) pixels
0x31A6, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x31A6, 0x0000, 0xFFFF, 0x0000,   // 0x0030 (48) pixels
0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x31A6, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x31A6, 0x0000,   // 0x0040 (64) pixels
0xFFFF, 0x0000, 0x31A6, 0xD69A, 0x31A6, 0x0000, 0xD69A, 0x31A6, 0xFFFF, 0x31A6, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x31A6,   // 0x0050 (80) pixels
0x31A6, 0x0000, 0x0000, 0xFFFF, 0x31A6, 0xFFFF, 0x31A6, 0x0000, 0xFFFF, 0x31A6, 0x0000, 0x31A6, 0xFFFF, 0xFFFF, 0x31A6, 0x0000,   // 0x0060 (96) pixels
};

const unsigned short humidity6x16[96] PROGMEM={
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xE71C, 0xE71C,   // 0x0010 (16) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xC638, 0xE71C, 0xE71C, 0xC638, 0x0000, 0x0000, 0xFFFF,   // 0x0020 (32) pixels
0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xE71C, 0xC638, 0x0000, 0x0000, 0xC638, 0xE71C,   // 0x0030 (48) pixels
0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000,   // 0x0040 (64) pixels
0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xE71C, 0xC638, 0x0000, 0x0000, 0xC638, 0xE71C, 0x0000, 0xE71C,   // 0x0050 (80) pixels
0xFFFF, 0xFFFF, 0xE71C, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0060 (96) pixels
};

typedef struct {
  const uint16_t  *data;
  uint16_t width;
  uint16_t height;
  uint8_t dataSize;
} tImage;
const tImage termo3 = {termo6x16, 6, 16, 8};
const tImage humidity = {humidity6x16, 6, 16, 8};

bool envEnabled = false;

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  delay(1500);
#endif
  initProperties();
  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, onIoTConnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, onIoTDisconnect);
  ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, onIoTSync);
}

void loop()
{
  unsigned long msNow = millis();
  ArduinoCloud.update();

  if (envEnabled && msNow - lastTimeUpdate >= 1000 )
  {
      displayDateTime();
      lastTimeUpdate = msNow;
  }

  //Check to see if data is available
  if (envEnabled && msNow - lastEnvironmentUpdate >= 10000 && myCCS811.dataAvailable())
  {
    myCCS811.readAlgorithmResults();
    const float humd = myHumidity.readHumidity();
    const float temp = myHumidity.readTemperature();
    //This sends the temperature data to the CCS811
    myCCS811.setEnvironmentalData(humd, temp);
    displayCO2Level(myCCS811.getCO2());
    displayTemperature((int) temp);
    displayHumidity((int) humd);
    cO2_Concentration_ppm = myCCS811.getCO2();
    temperature_combo_celsius =((int)(temp *10) )/ 10.0;
    // pressure_bar =(int) (myBME280.readFloatPressure()/100); //hpa
    relative_humidity = humd;
    temperature_imu_celsius = ((int)(IMU.readTempC()*10)) / 10.0;
    lastEnvironmentUpdate = msNow;
  }
  
}


void initEnvironmentalSensors(void) {
  Wire.begin(); //Inialize I2C Hardware
  if (myCCS811.begin() == false)
  {
#ifdef DEBUG    
    Serial.print("CCS811 error. Please check wiring. Freezing...");
#endif
    while (1);
  }
  myHumidity.begin();
}

void initWifi(void) {
  if (WiFi.status() == WL_NO_MODULE) {
#ifdef DEBUG 
    Serial.println("Communication with WiFi module failed!");
#endif
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
#ifdef DEBUG 
    Serial.println("Please upgrade the firmware");
#endif
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
#ifdef DEBUG 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
#endif
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
#ifdef DEBUG 
  Serial.println("Connected to WiFi");
  printWifiStatus();
#endif
}



// Display functions  -----------------------------------------------------------------


void initDisplay(void) {
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);    
  tft.drawRGBBitmap(3,135, (const uint16_t *)termo3.data, termo3.width, termo3.height); // Copy to screen
  tft.drawRGBBitmap( tft.width()/2+15,135, (const uint16_t *)humidity.data, humidity.width, humidity.height); // Copy to screen
}

void displayTemperature(const int temperature){
  tft.setTextSize(0);
  tft.setCursor(6,156);
  tft.setTextColor(ST77XX_GRAY_FA);
  tft.setFont(&FreeMonoBold9pt7b);
  drawUpdatedValue2d(temperature, 18 ,156);  
  tft.setFont(NULL);
  tft.print("C");
}

void displayHumidity(const int humidity) {
  tft.setTextSize(0);
  tft.setTextColor(ST77XX_GRAY_FA);
  tft.setFont(&FreeMonoBold9pt7b);
  drawUpdatedValue2d(humidity, tft.width()/2 + 28,156);
  tft.setFont(NULL);
  tft.print("%");
}

void displayCO2Level(const int co2level){
    tft.setFont(NULL);
    tft.setCursor(55,50);
    tft.setTextColor(ST77XX_GRAY_FA);
    tft.print("CO2");
    tft.setCursor(18,82);
    tft.setTextColor(ST77XX_GRAY_FA);
    tft.setTextSize(0);
    tft.setFont(&FreeMonoBoldOblique18pt7b);
    drawUpdatedValue4d(co2level,20,88);
    tft.setFont(NULL);
    tft.setCursor(88,90);
    tft.setTextColor(ST77XX_GRAY_FA);
    tft.print("ppm");
    drawGauge(co2level, 400, 8192);
    tft.setFont(NULL);
}

void drawUpdatedValue4d(const int number, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    char buffer[5]="6666";
    snprintf(buffer,sizeof(buffer), "%4d", number);
    tft.getTextBounds(buffer, x, y, &x1, &y1, &w, &h); //calc width of new string
    tft.fillRect(x,y-h,w+4, h+2,  ST77XX_BLACK);
    tft.setCursor(x,y);
    tft.print(buffer);
}

void drawUpdatedValue2d(const int number, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    char buffer[3]="66";
    snprintf(buffer,sizeof(buffer), "%2d", number);
    tft.getTextBounds(buffer, x, y, &x1, &y1, &w, &h); //calc width of new string
    tft.fillRect(x,y-h,w+4, h+2,  ST77XX_BLACK);
    tft.setCursor(x,y);
    tft.print(buffer);
}

// format and print time
void displayDateTime()
{
    char buf[8];
    sprintf(buf, "%.2d:%.2d", (rtc.getHours() + GMT) % 24, rtc.getMinutes());
    tft.fillRect(tft.width() - 33, 3,33,12,ST77XX_BLACK);
    tft.setFont(NULL);
    tft.setTextSize(0);
    tft.setCursor(tft.width() - 33, 3);
    tft.setTextColor(ST77XX_GRAY_FA);
    tft.print(buf);
}

void drawGauge (const int level, const int min, const int max) {
    static int lastValue;
    int degrees = (log(level)  / log(2) - log(min)  / log(2)) / (log(max)/log(2) - log(min)/log(2)) * 240;
    fillArc2(tft.width()/2, tft.height()/2-8, -120, 20, tft.width()/2-6, tft.width()/2-6, 6, GAUGE_GREEN);
    fillArc2(tft.width()/2, tft.height()/2-8,  -60, 20, tft.width()/2-6, tft.width()/2-6, 6, GAUGE_YELLOW);
    fillArc2(tft.width()/2, tft.height()/2-8,    0, 20, tft.width()/2-6, tft.width()/2-6, 6, GAUGE_ORANGE);
    fillArc2(tft.width()/2, tft.height()/2-8,   60, 20, tft.width()/2-6, tft.width()/2-6, 6, GAUGE_RED);
    fillArc2(tft.width()/2, tft.height()/2-8, lastValue-120, 3, tft.width()/2-16, tft.width()/2-16, 6, ST7735_BLACK );
    fillArc2(tft.width()/2, tft.height()/2-8, degrees-120,   3, tft.width()/2-16, tft.width()/2-16, 6, ST7735_WHITE );
    lastValue = degrees;
}

// #########################################################################
// Draw a circular or elliptical arc with a defined thickness
// #########################################################################

// x,y == coords of centre of arc
// start_angle = 0 - 359
// seg_count = number of 3 degree segments to draw (120 => 360 degree arc)
// rx = x axis radius
// yx = y axis radius
// w  = width (thickness) of arc in pixels
// colour = 16 bit colour value
// Note if rx and ry are the same then an arc of a circle is drawn

int fillArc2(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour)
{

  byte seg = 3; // Segments are 3 degrees wide = 120 segments for 360 degrees
  byte inc = 3; // Draw segments every 3 degrees, increase to 6 for segmented ring

    // Calculate first pair of coordinates for segment start
    float sx = cos((start_angle - 90) * DEG2RAD);
    float sy = sin((start_angle - 90) * DEG2RAD);
    uint16_t x0 = sx * (rx - w) + x;
    uint16_t y0 = sy * (ry - w) + y;
    uint16_t x1 = sx * rx + x;
    uint16_t y1 = sy * ry + y;

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

    // Copy segment end to sgement start for next segment
    x0 = x2;
    y0 = y2;
    x1 = x3;
    y1 = y3;
  }
}

// Debug helpers  -----------------------------------------------------------------
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


// Connection callbacks -----------------------------------------------------------------
void onIoTConnect() {
  // enable your other i2c devices
#ifdef DEBUG
  Serial.println(F(">>> connected to Arduino IoT Cloud"));
  Serial.println(F("enabling other i2c devices"));
#endif
  IMU.begin();
  initDisplay();
  initEnvironmentalSensors();
  envEnabled = true;
}

void onIoTDisconnect() {
  // disable your other i2c devices
#ifdef DEBUG
  Serial.println(F(">>> disconnected to Arduino IoT Cloud"));
  Serial.println(F("disabling other i2c devices"));
#endif
  // if necessary call the end() method on your i2c device library or even get rid of them.
  // really it's on a peripheral basis
  envEnabled = false;
}

void onIoTSync() {
#ifdef DEBUG
  Serial.println(">>> Board and Cloud SYNC OK");  
#endif
}

