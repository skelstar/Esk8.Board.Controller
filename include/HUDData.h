#ifndef ARDUINO_H
#include <Arduino.h>
#endif

class HUDData
{
public:
  uint32_t id;
  HUDCommand::Mode mode;
  HUDCommand::Colour colour;
  HUDCommand::Speed speed;

  HUDData(HUDCommand::Mode m, HUDCommand::Colour c, HUDCommand::Speed spd)
  {
    mode = m;
    colour = c;
    speed = spd;
  }
};
