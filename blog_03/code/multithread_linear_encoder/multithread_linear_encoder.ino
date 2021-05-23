/*
  Test Reed Switch Quadrature Encoder 02 With interrupts
  @author Enrique Albertos
  @link
  https://www.element14.com/community/community/design-challenges/design-for-a-cause-2021/

*/

// Port connections
int leftSwitchPort = 10; // switch to ground + internal pull-up resistor. Negative logic LOW when switch is closed  
int rightSwitchPort = 9;  // switch to ground + internal pull-up resistor. Negative logic LOW when switch is closed  
int leftLedPort = 8; // positive logic. HIGH turn on the LED  
int rightLedPort = 7; // positive logic. HIGH turn on the LED  


typedef enum direction_t  {RIGHT = 0x00, LEFT = 0xFF};
volatile direction_t lastWindowDirection = LEFT;

volatile int8_t encoderPosition = 0; // don't know where is our encoder, we'll need an absolute reference
                                     // let's assume we are in closed position and window opens from left to right
volatile bool encoderChangePending = false;
unsigned long lastLeftSwitchDebounceTime = 0;  // the last time the input left encoder pin was toggled
unsigned long debounceDelay = 50;    // the debounce time


void isrFallingLeftSwitchPort(); // ISR for leftSwitchPort

void setup() {
  // set port switches with internal 20K pull up resistors
  pinMode(leftSwitchPort, INPUT_PULLUP);
  pinMode(rightSwitchPort, INPUT_PULLUP);

  // detect falling edges the switch chages from open to closed. It is negative logic
  attachInterrupt(digitalPinToInterrupt(leftSwitchPort),
                  isrFallingLeftSwitchPort, FALLING);

  // set debugging LEDs ports
  pinMode(leftLedPort, OUTPUT);
  pinMode(rightLedPort, OUTPUT);

  Serial.begin(9600);
  delay(1000); // wait for serial port
}

void loop() {
  if (encoderChangePending) {
    encoderChangePending = false;
    // turn on left led when window direction is LEFT
    digitalWrite(leftLedPort, lastWindowDirection == LEFT );
     // and turn on right led when window direction is RIGHT
    digitalWrite(rightLedPort, lastWindowDirection == RIGHT );
    // log encoder position to serial port
    Serial.println(encoderPosition);
  }
}
/**
* Interrupt Service Routine for Falling Edge in Left Switch Port
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
