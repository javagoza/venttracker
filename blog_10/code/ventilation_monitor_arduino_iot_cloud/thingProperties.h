// Code generated by Arduino IoT Cloud, DO NOT EDIT.

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>


const char THING_ID[] = "7c03b195-xxxxxxxxxx"; // PLEASE adjust to your thing ID

const char SSID[]     = SECRET_SSID;    // Network SSID (name)
const char PASS[]     = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)

void onWindowAlertChange();

CloudSwitch windowOpen;
CloudSwitch windowAlert;
CloudPercentage windowPosition;
CloudLocation windowLocation;
CloudTemperature windowTemp;

void initProperties(){

  ArduinoCloud.setThingId(THING_ID);
  ArduinoCloud.addProperty(windowOpen, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(windowAlert, READWRITE, ON_CHANGE, onWindowAlertChange);
  ArduinoCloud.addProperty(windowPosition, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(windowLocation, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(windowTemp, READ, 120 * SECONDS, NULL);

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
