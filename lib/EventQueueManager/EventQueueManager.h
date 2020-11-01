#ifndef _EVENT_QUEUE_MANAGER_H_
#define _EVENT_QUEUE_MANAGER_H_

#include "Arduino.h"

class EventQueueManager
{

public:
  const uint8_t NO_MESSAGE_AVAILABLE = 99;

  EventQueueManager(QueueHandle_t queue, TickType_t ticks)
  {
    _queue = queue;
    _ticks = ticks;
  }

  template <typename T>
  void send(T ev)
  {
    uint8_t e = (uint8_t)ev;
    if (_queue != NULL)
      xQueueSendToBack(_queue, &e, _ticks);
  }

  bool messageAvailable()
  {
    HUDData peeked_val;
    return _queue != NULL && xQueuePeek(_queue, &peeked_val, _ticks) == pdTRUE;
  }

  // template <typename T>
  uint8_t read()
  {
    uint8_t e;
    if (_queue != NULL && xQueueReceive(_queue, &e, _ticks) == pdPASS)
    {
      return e;
    }
    return NO_MESSAGE_AVAILABLE;
  }

  void clear()
  {
  }

private:
  QueueHandle_t _queue = NULL;
  TickType_t _ticks = 10;
};

#endif