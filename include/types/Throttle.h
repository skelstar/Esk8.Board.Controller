#pragma once

#ifndef QUEUEBASE_CLASS
#include <QueueManager1.h>
#endif

//----------------------------------------
class ThrottleState : public QueueBase
{
public:
  ThrottleState() : QueueBase(event_id)
  {
    event_id = 0;
    name = "ThrottleState";
  }

  uint8_t val = 127;

  unsigned long event_id = 0;
};
