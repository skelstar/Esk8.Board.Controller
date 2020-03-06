#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <FSRThrottleLib.h>

uint8_t brakePin = 34, accelPin = 35;

uint8_t brakeIn[] = {0, 30, 70, 127};
uint8_t brakeOut[] = {0, 50, 90, 127};

uint8_t brakeInConservative[] = {0, 30, 70, 127};
uint8_t brakeOutConservative[] = {80, 90, 100, 127};

FSRPin brake(/*pin*/ 35, 1800, 4095, 0, 127);

uint8_t accelIn[4] = {127, 180, 200, 255};
uint8_t accelOut[4] = {127, 140, 170, 255};

FSRPin accel(/*pin*/ 34, 1800, 4095, 255, 127);

#define DEADMAN_PIN 0
Button2 deadman(DEADMAN_PIN);

FSRThrottleLib throttle(&accel, &brake, &deadman);

//------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);

  brake.setMaps(brakeIn, brakeOut);
  accel.setMaps(accelIn, accelOut);
}

elapsedMillis since_read_fsr;
bool updated = false;

void loop()
{
  if (since_read_fsr > 200)
  {
    since_read_fsr = 0;

    uint8_t t = throttle.get();
    throttle.print(/*width*/ 20);
  }

  // if (millis() > 10000 && !updated)
  // {
  //   updated = true;
  //   brake.setMaps(brakeInConservative, brakeOutConservative);
  //   DEBUG("updated map");
  // }
}