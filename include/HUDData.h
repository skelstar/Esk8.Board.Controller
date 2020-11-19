#ifndef ARDUINO_H
#include <Arduino.h>
#endif

enum HUDEvent
{
  HUD_EV_NO_COMMAND = 0,
  HUD_EV_PULSE_RED,
  HUD_EV_FLASH_GREEN,
};

const char *eventNames[] = {
    "EV_NO_COMMAND",
    "EV_PULSE_RED",
    "EV_FLASH_GREEN",
};

class HUDData
{
public:
  uint32_t id;
  HUDEvent state;
};

const char *eventToString(HUDEvent ev)
{
  return eventNames[(int)ev];
}