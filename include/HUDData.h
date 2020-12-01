#define HUDDATA_H

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
  uint8_t number;

  HUDData(HUDCommand::Mode m, HUDCommand::Colour c, HUDCommand::Speed spd, uint8_t num = 0)
  {
    mode = m;
    colour = c;
    speed = spd;
    number = num;
  }
};
