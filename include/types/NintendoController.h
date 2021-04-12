#pragma once

#include <types/QueueBase.h>
#include <QueueManager.h>

class NintendoButtonEvent : public QueueBase
{
public:
  uint8_t button;
  uint8_t state;
  unsigned long event_id, latency;

public:
  NintendoButtonEvent() : QueueBase(event_id, latency) {}

  NintendoButtonEvent(uint8_t button, uint8_t state) : QueueBase(event_id, latency)
  {
    button = button;
    state = state;
  }
};
