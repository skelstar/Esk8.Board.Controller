#pragma once
#include <shared-utils.h>

//------------------------------------------
enum PrimaryButtonEvent
{
  EV_NONE,
  EV_DOUBLE_CLICK,
  EV_TRIPLE_CLICK,
};

const char *getPrimaryButtonEvent(PrimaryButtonEvent ev)
{
  switch (ev)
  {
  case EV_NONE:
    return "EV_NONE";
  case EV_DOUBLE_CLICK:
    return "EV_DOUBLE_CLICK";
  case EV_TRIPLE_CLICK:
    return "EV_TRIPLE_CLICK";
  }
  return getOutOfRange("getPrimaryButtonEvent");
}
//------------------------------------------

class PrimaryButtonState : public QueueBase
{
public:
  bool pressed = false;
  float version = 1.00;
  PrimaryButtonEvent lastEvent = EV_NONE;

public:
  PrimaryButtonState() : QueueBase()
  {
    name = "PrimaryButtonState";
    event_id = 0;
  }
};
