#pragma once

#include <QueueManager1.h>

class DisplayEvent : public QueueBase
{
public:
  enum Event
  {
    NONE = 0,
    SOMETHING,
  };

  DisplayEvent() : QueueBase()
  {
    name = "DisplayEvent";
  }

  Event event = Event::NONE;
  int value = 0;
};
