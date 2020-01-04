#ifndef Arduino_h
#include <Arduino.h>
#endif

uint16_t trigger_centre = 0;
uint16_t trigger_max = 0;
uint16_t trigger_min = 0;
bool trigger_calibrated = false;

uint8_t deadzone = 3; // either side of trigger_centre

uint16_t get_trigger_raw()
{
  return analogRead(13);
}

uint8_t get_mapped_calibrated_throttle()
{
  if (trigger_calibrated == false)
  {
    return 127;
  }

  uint16_t raw = get_trigger_raw();

  if (raw > trigger_centre + deadzone)
  {
    return map(raw, trigger_centre, 4096, 127, 255);
  }
  else if (raw < trigger_centre - deadzone)
  {
    return map(raw, 0, trigger_centre, 0, 127);
  }
  return 127;
}