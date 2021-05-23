/**************************************************************************
  This is an example for testing the VenTTracker Window Sensor
  Displays the element14 logo in a Monochrome OLEDs based on SSD1306 drivers


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
    14 GND Power Power Ground to Battery (-)
    15 VIN Power In Vin Power input VIN to Baterry (+)
    20 D2 Digital GPIO - to Open/Closed Reed Switches
    25 D7 Digital; Right LED with a 100 Ohm resitor to ground
    26 D8 Digital GPIO to Left LED with a 100 Ohm resitor to ground
    27 D9/PWM Digital GPIO to  Right Reed Switch. Other end to ground.
    28 D10/PWM Digital GPIO;  to Left Reed Switch. Other end to ground.


  Author: Enrique Albertos
  Date: 2021-04-16
 **************************************************************************/


#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
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


#define LOGO_HEIGHT 20
#define LOGO_WIDTH 128
// element14 logo 8 pixels per Byte Little Endian Horizontal
static const unsigned char PROGMEM logo_bmp[] = {
    0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x60, 0x00, 0x00, 0x00, 0x07, 0x00, 0xE0, 0x1C, 0x00, 0x0C, 0x03, 0x00,
    0x07, 0x00, 0x01, 0x80, 0x70, 0x06, 0x18, 0x00, 0x3F, 0xE0, 0xE0, 0xFF,
    0x83, 0x3F, 0x9F, 0xE0, 0x3F, 0xE0, 0x6F, 0xF1, 0xFC, 0x06, 0x18, 0x00,
    0x70, 0x70, 0xE1, 0xC1, 0xC3, 0xE1, 0xF8, 0x70, 0x70, 0x70, 0x78, 0x78,
    0x70, 0x06, 0x18, 0x00, 0x60, 0x38, 0xE3, 0x80, 0xE3, 0xC0, 0xE0, 0x30,
    0x60, 0x38, 0x70, 0x18, 0x60, 0x06, 0x18, 0x00, 0xE0, 0x18, 0xE3, 0x00,
    0x63, 0x80, 0xE0, 0x30, 0xE0, 0x18, 0x60, 0x18, 0x60, 0x06, 0x18, 0x18,
    0xC0, 0x18, 0xE3, 0x00, 0x63, 0x80, 0xE0, 0x30, 0xC0, 0x18, 0x60, 0x18,
    0x60, 0x06, 0x18, 0x18, 0xC0, 0x18, 0xE3, 0x00, 0x63, 0x80, 0xE0, 0x30,
    0xC0, 0x18, 0x60, 0x18, 0x60, 0x06, 0x18, 0x18, 0xFF, 0xF8, 0xE3, 0xFF,
    0xE3, 0x80, 0xC0, 0x30, 0xFF, 0xF8, 0x60, 0x18, 0x60, 0x06, 0x18, 0x18,
    0xFF, 0xF8, 0xE3, 0xFF, 0xE3, 0x80, 0xC0, 0x30, 0xFF, 0xF8, 0x60, 0x18,
    0x60, 0x06, 0x18, 0x18, 0xC0, 0x00, 0xE3, 0x00, 0x03, 0x80, 0xC0, 0x30,
    0xC0, 0x00, 0x60, 0x18, 0x60, 0x06, 0x18, 0x18, 0xC0, 0x00, 0xE3, 0x00,
    0x03, 0x80, 0xC0, 0x30, 0xC0, 0x00, 0x60, 0x18, 0x60, 0x06, 0x1C, 0x18,
    0xE0, 0x00, 0xE3, 0x00, 0x03, 0x80, 0xC0, 0x30, 0xC0, 0x00, 0x60, 0x18,
    0x60, 0x06, 0x0F, 0xFF, 0xE0, 0x00, 0xE3, 0x80, 0x03, 0x80, 0xC0, 0x30,
    0xE0, 0x00, 0x60, 0x18, 0x60, 0x06, 0x03, 0xFE, 0x60, 0x00, 0xE1, 0x80,
    0x03, 0x80, 0xC0, 0x30, 0x60, 0x00, 0x60, 0x18, 0x70, 0x06, 0x00, 0x18,
    0x7F, 0xF0, 0xE1, 0xFF, 0xC3, 0x80, 0xC0, 0x30, 0x7F, 0xF0, 0x60, 0x18,
    0x3E, 0x06, 0x00, 0x18, 0x1F, 0xE0, 0x40, 0x7F, 0x83, 0x80, 0xC0, 0x30,
    0x1F, 0xE0, 0x60, 0x18, 0x1E, 0x06, 0x00, 0x18};


typedef enum direction_t { RIGHT = 0x00, LEFT = 0xFF };
volatile direction_t lastWindowDirection = LEFT;


volatile int8_t encoderPosition =
    0; // don't know where is our encoder, we'll need an absolute reference
// let's assume we are in closed position and window opens from left to right
volatile bool encoderChangePending = false;
unsigned long lastLeftSwitchDebounceTime =
    0; // the last time the input left encoder pin was toggled
unsigned long debounceDelay = 50; // the debounce time


void isrFallingLeftSwitchPort();      // ISR for leftSwitchPort
void isrChangeOpenClosedSwitchPort(); // ISR for open/close switches
void drawLogo(void); // draws logo_bmp centered on the OLED Display


void setup() {
  Serial.begin(9600);


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


  // After reset blink the logo
  display.clearDisplay();
  drawLogo();
  // Invert and restore display, pausing in-between
  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000);


  // detect falling edges the switch chages from open to closed. It is negative
  // logic
  attachInterrupt(digitalPinToInterrupt(leftSwitchPort),
                  isrFallingLeftSwitchPort, FALLING);


  // detect switch chages from open to closed. It is negative logic
  attachInterrupt(digitalPinToInterrupt(openClosedSwitchPort),
                  isrChangeOpenClosedSwitchPort, CHANGE);
}


void loop() {
  // if there is a new event from the encoder acknowledge it and do pending
  // actions
  if (encoderChangePending) {
    encoderChangePending = false;
    // turn on left led when window direction is LEFT
    digitalWrite(leftLedPort, lastWindowDirection == LEFT);
    // and turn on right led when window direction is RIGHT
    digitalWrite(rightLedPort, lastWindowDirection == RIGHT);
    // scroll the logo according direction
    if (lastWindowDirection == RIGHT) {
      display.startscrollright(0x00, 0x07);
    } else {
      display.startscrollleft(0x00, 0x07);
    }
    // log encoder position to serial port
    Serial.println(encoderPosition);
  }
}


/**
  Interrupt Service Routine for Falling Edge in Left Switch Port
*/
void isrFallingLeftSwitchPort() {
  if ((millis() - lastLeftSwitchDebounceTime) > debounceDelay) {
    if (digitalRead(rightSwitchPort) == HIGH) {
      encoderPosition++;
      lastWindowDirection = RIGHT;
    } else {
      encoderPosition--;
      lastWindowDirection = LEFT;
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
  int switchState = !digitalRead(openClosedSwitchPort);
  if ((millis() - lastLeftSwitchDebounceTime) > debounceDelay) {
    // Light LEDs when window closed
    digitalWrite(leftLedPort, switchState);
    digitalWrite(rightLedPort, switchState);
    if (switchState) {
      display.stopscroll();
    }
    lastLeftSwitchDebounceTime = millis();
  }
}


/**
  Send logo to the Display and center it then wait 1 second
*/
void drawLogo(void) {
  display.clearDisplay();
  display.drawBitmap((display.width() - LOGO_WIDTH) / 2,
                     (display.height() - LOGO_HEIGHT) / 2, logo_bmp, LOGO_WIDTH,
                     LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

