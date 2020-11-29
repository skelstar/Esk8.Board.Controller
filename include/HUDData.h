#ifndef ARDUINO_H
#include <Arduino.h>
#endif

class HUDData
{
public:
  uint32_t id;
  HUDCommand::Mode mode;
  HUDCommand::Colour colour;

  HUDData(HUDCommand::Mode m, HUDCommand::Colour c)
  {
    mode = m;
    colour = c;
  }
};
