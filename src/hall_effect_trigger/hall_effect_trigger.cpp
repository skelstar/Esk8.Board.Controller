#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#define HALL_EFFECT_TRIGGER_PIN 25

#define HALL_TRIGGER_MAX 2200
#define HALL_TRIGGER_MID 1833
#define HALL_TRIGGER_MIN 1740
#define HALL_TRIGGER_DEADBAND 10

//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  pinMode(HALL_EFFECT_TRIGGER_PIN, INPUT);
}

elapsedMillis since_read_trigger;

void loop()
{
  if (since_read_trigger > 200)
  {
    since_read_trigger = 0;

    uint16_t raw = analogRead(HALL_EFFECT_TRIGGER_PIN);
    uint16_t constrained = constrain(raw, HALL_TRIGGER_MIN, HALL_TRIGGER_MAX);

    uint8_t mapped = 127;
    if (constrained >= HALL_TRIGGER_MID + HALL_TRIGGER_DEADBAND)
    {
      mapped = map(constrained, HALL_TRIGGER_MID, HALL_TRIGGER_MAX, 127, 0);
    }
    else if (constrained <= HALL_TRIGGER_MID - HALL_TRIGGER_DEADBAND)
    {
      mapped = map(constrained, HALL_TRIGGER_MIN, HALL_TRIGGER_MID, 255, 127);
    }

    DEBUGVAL(raw, constrained, mapped);
  }
}