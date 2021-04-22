#pragma once

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
