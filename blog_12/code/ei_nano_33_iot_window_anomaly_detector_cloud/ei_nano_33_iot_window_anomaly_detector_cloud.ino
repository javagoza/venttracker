/* 
 *  ei_nano_33_iot_accelerometer_demo
 *  Modified by :Enrique Albertos
 *  Date: 2021-04-11
 *  
 * Edge Impulse Arduino examples
 * Copyright (c) 2021 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* needed for Arduino nano 33 IoT with Arduino IDE -------------------------- */
#include <cstdarg>
#define EIDSP_USE_CMSIS_DSP             1  
#define EIDSP_LOAD_CMSIS_DSP_SOURCES    1  
#define __STATIC_FORCEINLINE                   __attribute__((always_inline)) static inline  
#define __SSAT(ARG1, ARG2) \  
__extension__ \  
({                          \  
  int32_t __RES, __ARG1 = (ARG1); \  
  __ASM volatile ("ssat %0, %1, %2" : "=r" (__RES) :  "I" (ARG2), "r" (__ARG1) : "cc" ); \  
  __RES; \  
 })  
  

/* Includes ---------------------------------------------------------------- */
#include <venttracker-window-anomaly-detector_inference.h>
#include "arduino_secrets.h"
#include "thingProperties.h"
#include <Arduino_LSM6DS3.h>


#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

/* Constant defines -------------------------------------------------------- */
#define CONVERT_G_TO_MS2    9.80665f

/* Private variables ------------------------------------------------------- */
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

bool envEnabled = false;

/**
* @brief      Arduino setup function
*/
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.println("Edge Impulse Inferencing Demo");
    delay(1500); // wait for serial

    initProperties();
    
    // Connect to Arduino IoT Cloud
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  
  
    ArduinoCloud.addCallback(ArduinoIoTCloudEvent::CONNECT, onIoTConnect);
    ArduinoCloud.addCallback(ArduinoIoTCloudEvent::DISCONNECT, onIoTDisconnect);
    ArduinoCloud.addCallback(ArduinoIoTCloudEvent::SYNC, onIoTSync);
  
  
    //setDebugMessageLevel(2);
    delay(3000);
    ArduinoCloud.printDebugInfo();

}

/**
* @brief      Printf function uses vsnprintf and output using Arduino Serial
*
* @param[in]  format     Variable argument list
*/
void ei_printf(const char *format, ...) {
   static char print_buf[1024] = { 0 };

   va_list args;
   va_start(args, format);
   int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
   va_end(args);

   if (r > 0) {
       Serial.write(print_buf);
   }
}

/**
* @brief      Get data and run inferencing
*
* @param[in]  debug  Get debug info if true
*/
void loop()
{
  unsigned long msNow = millis();
  ArduinoCloud.update();
  delay(2000);
  //Check to see if data is available
   if (envEnabled) {
    ei_printf("Sampling...\n");

    // Allocate a buffer here for the values we'll read from the IMU
    float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 };

    for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
        // Determine the next tick (and then sleep later)
        uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

        IMU.readAcceleration(buffer[ix], buffer[ix + 1], buffer[ix + 2]);

        buffer[ix + 0] *= CONVERT_G_TO_MS2;
        buffer[ix + 1] *= CONVERT_G_TO_MS2;
        buffer[ix + 2] *= CONVERT_G_TO_MS2;

        delayMicroseconds(next_tick - micros());
    }

    // Turn the raw buffer in a signal which we can the classify
    signal_t signal;
    int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
    if (err != 0) {
        ei_printf("Failed to create signal from buffer (%d)\n", err);
        return;
    }

    // Run the classifier
    ei_impulse_result_t result = { 0 };

    err = run_classifier(&signal, &result, debug_nn);
    if (err != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", err);
        return;
    }

    // print the predictions
    ei_printf("Predictions ");
    ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
        result.timing.dsp, result.timing.classification, result.timing.anomaly);
    ei_printf(": \n");
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s: ", result.classification[ix].label);
        Serial.println(result.classification[ix].value, 5);
        
    }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: " );
    Serial.println(result.anomaly, 3);
#endif
    }
}



void onIoTConnect() {
  // enable your other i2c devices
  Serial.println(F(">>> connected to Arduino IoT Cloud"));

    delay(1500); // wait for serial

    if (!IMU.begin()) {
        ei_printf("Failed to initialize IMU!\r\n");
    }
    else {
        ei_printf("IMU initialized\r\n");
    }
   
  envEnabled = true;
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

void onWindowAlertChange(){
  Serial.println(">>> on Window Alert Change");  
}
