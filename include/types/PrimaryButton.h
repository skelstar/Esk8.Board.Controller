#pragma once

#include <QueueManager.h>

//------------------------------------------
class PrimaryButtonState : public QueueBase
{
public:
  PrimaryButtonState() : QueueBase(event_id)
  {
    event_id = 0;
    name = "PrimaryButtonState";
  }

  bool pressed = false;
  unsigned long event_id = 0;
};
