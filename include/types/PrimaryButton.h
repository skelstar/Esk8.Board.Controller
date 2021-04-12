#pragma once

#include <QueueManager.h>

//------------------------------------------
class PrimaryButtonState : public QueueBase
{
public:
  bool pressed = false;
  unsigned long event_id = 0,
                latency = 0;

public:
  PrimaryButtonState() : QueueBase(event_id, latency)
  {
    event_id = 0;
    name = "PrimaryButtonState";
  }
};
