#pragma once

#include <QueueManager.h>

//------------------------------------------
class PrimaryButtonState : public QueueBase
{
public:
  bool pressed = false;

public:
  PrimaryButtonState() : QueueBase()
  {
    name = "PrimaryButtonState";
    event_id = 0;
  }
};
