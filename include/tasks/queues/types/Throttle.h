#pragma once

//----------------------------------------
class ThrottleState : public QueueBase
{
public:
  uint8_t val = 127;
  uint8_t status = 0; // OK

public:
  ThrottleState() : QueueBase()
  {
    event_id = 0;
    name = "ThrottleState";
    status = 0; // OK
  }
};
