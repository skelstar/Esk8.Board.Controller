#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

// https://www.onsemi.com/pub/Collateral/2N7000-D.PDF

#define HAPTIC_PIN 26

void setup()
{
  Serial.begin(115200);

  pinMode(HAPTIC_PIN, OUTPUT);
}

int i = 0;
elapsedMillis since_haptic_event = 0;

void loop()
{
  if (since_haptic_event > 3000)
  {
    since_haptic_event = 0;
    digitalWrite(HAPTIC_PIN, HIGH);

    while (since_haptic_event < 200)
    {
      delay(1);
    }
    digitalWrite(HAPTIC_PIN, LOW);
  }
}