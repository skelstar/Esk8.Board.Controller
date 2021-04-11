#pragma once

#include <QueueManager.h>

class NintendoButtonEvent : public QueueBase
{
public:
  uint8_t button;
  uint8_t state;

  NintendoButtonEvent() {}

  NintendoButtonEvent(uint8_t button, uint8_t state)
  {
    button = button;
    state = state;
  }
};
