/*
  Test Reed Switch Quadrature Encoder
  @author Enrique Albertos
  @link https://www.element14.com/community/community/design-challenges/design-for-a-cause-2021/

*/

// Port connections
int leftSwitchPort = 10; // switch to ground + internal pull-up resistor. Negative logic LOW when switch is closed  
int rightSwitchPort = 9;  // switch to ground + internal pull-up resistor. Negative logic LOW when switch is closed  
int leftLedPort = 8; // positive logic. HIGH turn on the LED  
int rightLedPort = 7; // positive logic. HIGH turn on the LED  

void setup() {
  // set port switches with internal 20K pull up resistors
  pinMode (leftSwitchPort, INPUT_PULLUP);
  pinMode (rightSwitchPort, INPUT_PULLUP);

  // set debugging LEDs ports
  pinMode (leftLedPort, OUTPUT);
  pinMode (rightLedPort, OUTPUT);
  
  Serial.begin (9600);
  delay(1000); // wait for serial port
}

void loop() {
  static int encoderPosition = 0;
  static int leftSwitchPortLast = LOW;
  // switches are in negative logic. Negate
  int leftSwitchState = !digitalRead(leftSwitchPort);
  int rightSwitchState = !digitalRead(rightSwitchPort);
  
  // Light leds for debugging
  digitalWrite(leftLedPort, leftSwitchState);
  digitalWrite(rightLedPort, rightSwitchState);
  if ((leftSwitchPortLast == LOW) && (leftSwitchState == HIGH)) {
    if (rightSwitchState == LOW) {
      encoderPosition++;
    } else {
      encoderPosition--;
    }
     Serial.println(encoderPosition);
  }
  leftSwitchPortLast = leftSwitchState;
  // debouncing switches
  delay(20);
}
