#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <FSRThrottleLib.h>

#define FSR_BRAKE_PIN 36
#define FSR_ACCEL_PIN 39

uint8_t brakeIn[] = {0, 30, 70, 127};
uint8_t brakeOut[] = {0, 50, 90, 127};

// maps
uint8_t brakeInConservative[] = {0, 30, 70, 127};
uint8_t brakeOutConservative[] = {80, 90, 100, 127};
uint8_t accelIn[4] = {127, 180, 200, 255};
uint8_t accelOut[4] = {127, 140, 170, 255};

#define DEADMAN_PIN 0
Button2 deadman(DEADMAN_PIN);

FSRPin brake(/*pin*/ FSR_BRAKE_PIN, FSR_MIN_RAW, FSR_MAX_RAW, 0, 127);
FSRPin accel(/*pin*/ FSR_ACCEL_PIN, FSR_MIN_RAW, FSR_MAX_RAW, 255, 127);

FSRThrottleLib throttle(&accel, &brake, &deadman);

//------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);
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
}