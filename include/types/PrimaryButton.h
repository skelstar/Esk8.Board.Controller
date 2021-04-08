#pragma once

#include <QueueManager.h>

//------------------------------------------
class PrimaryButtonState : public QueueBase
{
public:
  PrimaryButtonState()
  {
    event_id = 0;
  }

  bool pressed = false;
};
