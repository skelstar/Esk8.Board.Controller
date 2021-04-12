#pragma once

#include <types/QueueBase.h>
#include <QueueManager.h>

class NintendoButtonEvent : public QueueBase
{
public:
  uint8_t button;
  uint8_t state;
  unsigned long event_id, latency;
  const char *name = "NintendoButtonEvent";

public:
  NintendoButtonEvent() : QueueBase(event_id, latency) {}

  NintendoButtonEvent(uint8_t button, uint8_t state) : QueueBase(event_id, latency)
  {
    button = button;
    state = state;
  }
};
