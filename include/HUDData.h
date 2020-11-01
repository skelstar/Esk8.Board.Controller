#ifndef ARDUINO_H
#include <Arduino.h>
#endif

enum HUDEvent
{
  HUD_EV_NO_COMMAND = 0,
  HUD_EV_PULSE,
  HUD_EV_BOARD_CONNECTED,
  HUD_EV_BOARD_DISCONNECTED,
  HUD_EV_BOARD_MOVING,
  HUD_EV_BOARD_STOPPED
};

class HUDData
{
public:
  uint32_t id;
  HUDEvent state;
};

const char *eventToString(HUDEvent ev)
{
  switch (ev)
  {
  case HUD_EV_NO_COMMAND:
    return "NO_COMMAND";
  case HUD_EV_PULSE:
    return "PULSE";
  case HUD_EV_BOARD_CONNECTED:
    return "BOARD_CONNECTED";
  case HUD_EV_BOARD_DISCONNECTED:
    return "BOARD_DISCONNECTED";
  case HUD_EV_BOARD_MOVING:
    return "BOARD_MOVING";
  case HUD_EV_BOARD_STOPPED:
    return "BOARD_STOPPED";
  default:
    return "ERROR: unhandled event!";
  }
}