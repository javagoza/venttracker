/*
  Testing Casement Window and Door Orientation with Arduino nano 33 IoT LSM6DS3 IMU and Madgwick filter
  @author Enrique Albertos
  @link https://www.element14.com/community/community/design-challenges/design-for-a-cause-2021/

*/

#include <Arduino_LSM6DS3.h>
#include <MadgwickAHRS.h>

Madgwick filter;

void setup() {
  Serial.begin(9600);
  delay(1000);
  // start the IMU and filter
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  filter.begin(IMU.accelerationSampleRate());
}

void loop() {
  float  aix, aiy, aiz;
  float gix, giy, giz;
  float roll, pitch, heading;

  // read raw data from IMU
  if (IMU.accelerationAvailable() && IMU.gyroscopeAvailable()) {
    IMU.readAcceleration(aix, aiy, aiz);
    IMU.readGyroscope(gix, giy, giz);
    filter.updateIMU(0, giy, 0, 0, aiz, 0);
    heading = -filter.getYaw();
    Serial.print("Orientation: ");
    Serial.println(heading);
  }

}



