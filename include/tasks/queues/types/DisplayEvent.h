#pragma once

#include <QueueBase.h>

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
