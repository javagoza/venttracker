#include "arduino_secrets.h"
/* 
  This is an example for testing the VenTTracker Window Sensor
  connecte to Arduino IoT Cloud.
  Displays a progress bar indicating the window openes on 
  a Monochrome OLED display based on SSD1306 drivers.
  This example is for a 128x32 pixel display using I2C to communicate
  3 pins are required to interface (two I2C and one reset).

  Lights LED according sensor events:
                       | LEFT LED  |  RIGHT LED  |
  WINDOW OPEN EVENT    |    OFF    |    OFF      |
  WINDOW CLOSED EVENT  |    ON     |    ON       |
  TO THE LEFT MOVEMENT |    ON     |    OFF      |
  TO THE RIGHT MOVEMENT|    ON     |    OFF      |

  Hardware. Pinout
    8 A4/SDA Analog ADC in; I2C SDA; SDA to SDA OLED Display
    9 A5/SCL Analog ADC in; I2C SCL to SCK OLED Display
    13 RST Digital In Active low reset input  RESET to push button. Other end to ground.
    14 GND Power Power Ground to Battery  - 
    15 VIN Power In Vin Power input VIN to Baterry  + 
    20 D2 Digital GPIO - to Open/Closed Reed Switches
    25 D7 Digital; Right LED with a 100 Ohm resitor to ground
    26 D8 Digital GPIO to Left LED with a 100 Ohm resitor to ground
    27 D9/PWM Digital GPIO to  Right Reed Switch. Other end to ground.
    28 D10/PWM Digital GPIO;  to Left Reed Switch. Other end to ground.

  Arduino IoT Cloud Thing "VT-CR01-WW01"
  https://create.arduino.cc/cloud/things/7c03b195-29d1-42ba-92e1-651a8e7d4010 

  Cloud Variables:

  CloudSwitch windowOpen: (RO) indicates if the window is open or closed
  CloudSwitch windowAlert; (RW) can signal the window in alert state
  CloudPercentage windowPosition; (RO) indicates if the window open percentage (0%, 25%, 50%, 75%, 100%)
  CloudLocation windowLocation; (RO) indicates window geolocation coordinates
  CloudTemperature windowTemp; (RO) indicates window sensor IMU temperature.

  Author: Enrique Albertos
  Date: 2021-05-01


*/

#include "thingProperties.h"
#include "SPI.h"
#include "Wire.h"
#include "SparkFunLSM6DS3.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

int windowState = HIGH;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Port connections
int leftSwitchPort = 10;      // switch to ground + internal pull-up resistor.
                              // Negative logic LOW when switch is closed
int rightSwitchPort = 9;      // switch to ground + internal pull-up resistor.
                              // Negative logic LOW when switch is closed
int leftLedPort = 8;          // positive logic. HIGH turn on the LED
int rightLedPort = 7;         // positive logic. HIGH turn on the LED
int openClosedSwitchPort = 2; // switch to ground + internal pull-up resistor.
                              // Negative logic LOW when switch is closed

typedef enum direction_t { RIGHT = 0x00, LEFT = 0xFF };
volatile direction_t lastWindowDirection = LEFT;

volatile int8_t encoderPosition =  0; // don't know where is our encoder, we'll need an absolute reference
// let's assume we are in closed position and window opens from left to right
volatile bool encoderChangePending = false;
volatile bool openCloseChangePending = true;
unsigned long lastLeftSwitchDebounceTime = 0; // the last time the input left encoder pin was toggled
unsigned long debounceDelay = 150; // the debounce time

#define LOGO_HEIGHT   32
#define LOGO_WIDTH    64
//  logo 8 pixels per Byte Little Endian Horizontal
static const unsigned char PROGMEM logoDesignForACause[] ={
0x00, 0x00, 0x0F, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xC0, 0x01, 0xFE, 0x00, 0x00,
0x00, 0x00, 0xFF, 0xF8, 0x07, 0xFF, 0x80, 0x00, 0x00, 0x03, 0xFF, 0xFC, 0x1F, 0xFF, 0xE0, 0x00,
0x00, 0x07, 0xFF, 0xFE, 0x3F, 0xFF, 0xF0, 0x00, 0x00, 0x0F, 0xFF, 0xFE, 0x3F, 0xFF, 0xFC, 0x00,
0x00, 0x1F, 0xF9, 0xFE, 0x3F, 0xEF, 0xFC, 0x00, 0x00, 0x3F, 0xF0, 0x3E, 0x3E, 0x07, 0xFE, 0x00,
0x00, 0x7F, 0xE0, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x7F, 0x80, 0x00, 0x00, 0x00, 0xFF, 0x00,
0x00, 0xFF, 0x80, 0xF0, 0x0F, 0x10, 0xFF, 0x80, 0x00, 0xFF, 0x83, 0xFC, 0x3F, 0xC0, 0xFF, 0xC0,
0x01, 0xFF, 0x07, 0x1E, 0x70, 0xE0, 0x7F, 0xC0, 0x01, 0xFF, 0x0E, 0x07, 0xE0, 0x60, 0x7F, 0xE0,
0x03, 0xFF, 0x0C, 0x03, 0xC4, 0x30, 0x7F, 0xE0, 0x03, 0xFE, 0x0C, 0xE1, 0x8E, 0x30, 0x3F, 0xE0,
0x07, 0xFF, 0x0C, 0x03, 0x86, 0x30, 0x7F, 0xF0, 0x07, 0xFF, 0x0C, 0x03, 0xC4, 0x70, 0x7F, 0xF0,
0x0F, 0xFF, 0x8E, 0x07, 0xE0, 0x60, 0xFF, 0xF8, 0x1F, 0xFF, 0x87, 0xBE, 0x79, 0xE0, 0xFF, 0xFC,
0x1F, 0xFF, 0xC3, 0xF8, 0x3F, 0x81, 0xFF, 0xFC, 0x3F, 0xFF, 0xE0, 0x60, 0x06, 0x03, 0xFF, 0xFE,
0x7F, 0xFF, 0xF8, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x3F, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xF0, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x1F, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFE, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x06, 0x30, 0x07, 0xFF, 0xFF,
0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF
};

#define TITLE_HEIGHT   32
#define TITLE_WIDTH    112
// title logo 8 pixels per Byte Little Endian Horizontal
static const unsigned char PROGMEM designChallengeTitle[] ={
0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00,
0x01, 0x80, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00,
0x00, 0x06, 0x00, 0x00, 0x00, 0x0C, 0xC0, 0x00, 0x00, 0x00, 0x67, 0x3C, 0x79, 0x1F, 0x3E, 0x0F,
0x3C, 0xD0, 0x38, 0x18, 0xCF, 0x11, 0x1C, 0x78, 0x63, 0x7E, 0xD9, 0x9F, 0x3E, 0x0F, 0x6E, 0xF0,
0x6C, 0x18, 0x1F, 0xBB, 0x36, 0x7C, 0x63, 0x66, 0xC9, 0xB3, 0x37, 0x06, 0x66, 0xE0, 0x0C, 0x18,
0x01, 0xBB, 0x72, 0xCC, 0x63, 0x7E, 0xE1, 0xB3, 0x37, 0x06, 0x66, 0xC0, 0x3C, 0x18, 0x07, 0xBB,
0x38, 0xFC, 0x63, 0x7E, 0x79, 0xB3, 0x37, 0x06, 0x66, 0xC0, 0x7C, 0x18, 0xDF, 0xBB, 0x1E, 0xFC,
0x67, 0x60, 0x1D, 0xB3, 0x37, 0x06, 0x66, 0xC0, 0xCC, 0x18, 0xD9, 0xBB, 0x06, 0xC0, 0x66, 0x66,
0xDD, 0x9F, 0x37, 0x06, 0x66, 0xC0, 0xEC, 0x0D, 0xD9, 0xBB, 0x76, 0xEC, 0x7C, 0x3C, 0xF9, 0x9F,
0x37, 0x06, 0x7C, 0xC0, 0x7C, 0x0F, 0x9F, 0x9F, 0x3E, 0x78, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x0E, 0x10, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F,
0x30, 0x00, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0xB0, 0x00,
0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0xBF, 0x1E, 0x66, 0x38,
0xB8, 0xF8, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x3F, 0x3F, 0x66, 0x7D, 0xF8, 0xF9,
0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x33, 0x03, 0x66, 0xCC, 0xCD, 0x99, 0xB0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x33, 0x1F, 0x66, 0xFC, 0xCD, 0x9B, 0xF8, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x31, 0xB3, 0x3B, 0x66, 0xE1, 0xCD, 0x9B, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x39, 0xB3, 0x33, 0x66, 0xC4, 0xCD, 0x99, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B,
0xB3, 0x37, 0x66, 0xEC, 0xCC, 0xF9, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x33, 0x3F,
0x66, 0x7C, 0xCC, 0xF9, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xF8,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// double buffer for the display
GFXcanvas1 canvas(SCREEN_WIDTH, SCREEN_HEIGHT); // 128x32 pixel canvas

void isrFallingLeftSwitchPort();      // ISR for leftSwitchPort
void isrChangeOpenClosedSwitchPort(); // ISR for open/close switches


LSM6DS3 IMU(I2C_MODE, 0x6A);

void setup() {

  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(5000); 
  setupAccelerometer();
  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
 */
  setDebugMessageLevel(4);
  ArduinoCloud.printDebugInfo();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // set port switches with internal 20K pull up resistors
  pinMode(leftSwitchPort, INPUT_PULLUP);
  pinMode(rightSwitchPort, INPUT_PULLUP);
  pinMode(openClosedSwitchPort, INPUT_PULLUP);

  // set debugging LEDs ports
  pinMode(leftLedPort, OUTPUT);
  pinMode(rightLedPort, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  blinkLogo(designChallengeTitle, TITLE_WIDTH, TITLE_HEIGHT);
  blinkLogo(logoDesignForACause, LOGO_WIDTH, LOGO_HEIGHT);
  // detect falling edges the switch chages from open to closed. It is negative
  // logic
  attachInterrupt(digitalPinToInterrupt(leftSwitchPort),
                  isrFallingLeftSwitchPort, FALLING);

  // detect switch chages from open to closed. It is negative logic
  attachInterrupt(digitalPinToInterrupt(openClosedSwitchPort),
                  isrChangeOpenClosedSwitchPort, CHANGE);

}

void loop() {
  ArduinoCloud.update();
  windowLocation =  Location(41.652785957455f,  -0.8729272143593402f);
  windowOpen = !windowState;
  windowTemp = IMU.readTempC();
  if (windowState == HIGH) {
    windowPosition = 0;
  } else {
    windowPosition = (encoderPosition + 1) * 25;
  }
  // if there is a new event from the encoder acknowledge it and do pending
  // actions
  if (encoderChangePending) {
    encoderChangePending = false;
    // turn on left led when window direction is LEFT
    digitalWrite(leftLedPort, lastWindowDirection == LEFT);
    // and turn on right led when window direction is RIGHT
    digitalWrite(rightLedPort, lastWindowDirection == RIGHT);
    // log encoder position to serial port
    Serial.println(encoderPosition); 
    if (windowState == LOW) {
      drawProgressBar(encoderPosition);
    } else {
      drawLogo(logoDesignForACause, LOGO_WIDTH, LOGO_HEIGHT);
    }   
  }
  if (openCloseChangePending) {
    openCloseChangePending = false;
        // Light LEDs when window closed
    digitalWrite(leftLedPort, windowState);
    digitalWrite(rightLedPort, windowState);
    if (windowState == LOW) {
      drawProgressBar(encoderPosition);
      windowAlert = false;
    } else {
      drawProgressBar(-1);
    }
  }
  if(windowAlert){ // blink leds
    digitalWrite(leftLedPort, true);
    digitalWrite(rightLedPort, true);
    delay(400);
    digitalWrite(leftLedPort, false);
    digitalWrite(rightLedPort, false);
    delay(400);
  }
  
}

void setupAccelerometer() {
  IMU.begin();
}




void onWindowAlertChange() {
  Serial.print("On Window Change\n");
  if(windowAlert) {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    displayOpenWindow();
  } else { 
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
 
  }
  
}


/**
  Interrupt Service Routine for Falling Edge in Left Switch Port
*/
void isrFallingLeftSwitchPort() {
  if ((millis() - lastLeftSwitchDebounceTime) > debounceDelay) {
    if (digitalRead(rightSwitchPort) == HIGH) {
      if (encoderPosition > 0) {
        encoderPosition--;
      }
      lastWindowDirection = LEFT;
    } else {
      if (encoderPosition < 3) {
        encoderPosition++;
      }
      lastWindowDirection = RIGHT;
    }
    lastLeftSwitchDebounceTime = millis();
    encoderChangePending = true;
  }
}

/**
  Interrupt Service Routine for Changing Edge in Open Close Switches
  when the window is closed light the two LEDS
*/
void isrChangeOpenClosedSwitchPort() {
  windowState = !digitalRead(openClosedSwitchPort);
  if ((millis() - lastLeftSwitchDebounceTime) > debounceDelay) {
    encoderPosition = 0;
    openCloseChangePending = true;
    lastLeftSwitchDebounceTime = millis();
  }
}

/**
* Print the progress bar
*/
void drawProgressBar(const int counter) {
  display.clearDisplay();
  canvas.fillScreen(BLACK);
  canvas.setCursor(0, 0);
  canvas.setTextSize(2);
  canvas.print((counter+1)*25);
  canvas.print("% ");
  canvas.print(IMU.readTempC(),1);
  canvas.print("C");
  for(int i = 0; i < 4; ++i) {
    canvas.drawRect(i * SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2,SCREEN_WIDTH /4 , SCREEN_HEIGHT / 2 , WHITE  );
  }  
  for (int i = 0 ; i <= counter;++i) {
      canvas.fillRect(i * SCREEN_WIDTH / 4+3, SCREEN_HEIGHT / 2 + 4,SCREEN_WIDTH /4 -6, SCREEN_HEIGHT / 2 -8, WHITE  );
  }
  display.drawBitmap(0, 0, canvas.getBuffer(), 128, 32, WHITE,
                     BLACK); // Copy to screen
  display.display();
}

/**
* Print the progress bar
*/
void displayOpenWindow() {
  display.clearDisplay();
  canvas.fillScreen(BLACK);
  canvas.setCursor(0, 0);
  canvas.setTextSize(2);
  canvas.println("  PLEASE,");
  canvas.print(  "   OPEN ");
  display.drawBitmap(0, 0, canvas.getBuffer(), 128, 32, WHITE,
                     BLACK); // Copy to screen
  display.display();
}


/**
  Send a logo to the Display and center it and wait 1 second
*/
void drawLogo(const unsigned char* logo, const int width, const int height ) {
  display.clearDisplay();
  display.drawBitmap(
    (display.width()  - width ) / 2,
    (display.height() - height) / 2,
    logo, width, height, 1);
  display.display();
}

/**
  blinks a logo on the display
*/
void blinkLogo(const unsigned char* logo, const int width, const int height) {
    // After reset blink the logo
  display.clearDisplay();
  drawLogo(logo, width, height);
  //Invert and restore display, pausing in-between
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(2000);
}


