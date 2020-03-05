#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <FSRThrottleLib.h>

uint8_t brakePin = 34, accelPin = 35;

uint8_t brakeIn[] = {0, 30, 70, 127};
uint8_t brakeOut[] = {0, 50, 90, 127};
FSRPin brake(/*pin*/ 35, 1800, 4095, 0, 127, brakeIn, brakeOut);

uint8_t accelIn[] = {127, 180, 200, 255};
uint8_t accelOut[] = {127, 140, 170, 255};
FSRPin accel(/*pin*/ 34, 1800, 4095, 255, 127, accelIn, accelOut);

FSRThrottleLib throttle(accel, brake);

//------------------------------------------------------------

void setup(void)
{
  Serial.begin(115200);
  Serial.println("**** I2C Encoder V2 basic example ****");

  pinMode(brakePin, INPUT);
  pinMode(accelPin, INPUT);
}

elapsedMillis since_read_fsr;

const uint16_t brakeMin = 1700, brakeMax = 4095;
const uint16_t accelMin = 1700, accelMax = 4095;

void loop()
{
  if (since_read_fsr > 200)
  {
    since_read_fsr = 0;

    throttle.get();
    throttle.print(/*width*/ 20);
  }
}