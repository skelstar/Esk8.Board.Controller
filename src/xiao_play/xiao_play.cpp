#include <Arduino.h>
#include <elapsedMillis.h>

void setup()
{
  // Serial.begin(115200);
}

int i = 0;
elapsedMillis since_haptic_event = 0;

void loop()
{
  if (since_haptic_event > 3000)
  {
    // DEBUG("ping");
  }
}