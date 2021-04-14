#pragma once

#include <types/QueueBase.h>
#include <QueueManager.h>

class NintendoButtonEvent : public QueueBase
{
public:
  uint8_t button;
  uint8_t state;

public:
  NintendoButtonEvent() : QueueBase()
  {
    name = "NintendoButtonEvent";
  }

  NintendoButtonEvent(uint8_t button, uint8_t state) : QueueBase()
  {
    button = button;
    state = state;
    name = "NintendoButtonEvent";
  }
};
