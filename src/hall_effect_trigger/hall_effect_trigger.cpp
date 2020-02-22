#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>
#include <HallEffectThrottleLib.h>

#define HALL_EFFECT_TRIGGER_PIN 36

//------------------------------------------------------------------

HallEffectThrottleLib trigger;

void setup()
{
  Serial.begin(115200);

  trigger.init(HALL_EFFECT_TRIGGER_PIN, /*braking*/ 15, /*accel*/ 15);
  trigger.getMiddle();
}

elapsedMillis since_read_trigger;
uint8_t old_throttle;

void loop()
{
  if (since_read_trigger > 200)
  {
    since_read_trigger = 0;

    uint8_t throttle = trigger.getThrottle();
    if (old_throttle != throttle)
    {
      trigger.print();
    }

    old_throttle = throttle;
  }
}