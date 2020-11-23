#ifndef ARDUINO_H
#include <Arduino.h>
#endif

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