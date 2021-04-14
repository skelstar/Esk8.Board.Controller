#pragma once

#ifndef QUEUEBASE_CLASS
#include <QueueManager1.h>
#endif

//----------------------------------------
class ThrottleState : public QueueBase
{
public:
  uint8_t val = 127;

public:
  ThrottleState() : QueueBase()
  {
    event_id = 0;
    name = "ThrottleState";
  }
};
