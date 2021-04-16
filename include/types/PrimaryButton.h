#pragma once

#include <QueueManager.h>

//------------------------------------------
class PrimaryButtonState : public QueueBase
{
public:
  bool pressed = false;
  float version = 1.00;

public:
  PrimaryButtonState() : QueueBase()
  {
    name = "PrimaryButtonState";
    event_id = 0;
  }
};
