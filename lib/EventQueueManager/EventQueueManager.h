#ifndef _EVENT_QUEUE_MANAGER_H_
#define _EVENT_QUEUE_MANAGER_H_

#ifndef ARDUINO_H
#include "Arduino.h"
#endif

#define NO_QUEUE_EVENT 99

class EventQueueManager
{

public:
  EventQueueManager(QueueHandle_t queue, TickType_t ticks)
  {
    _queue = queue;
    _ticks = ticks;
  }

  /// send event T to the queue
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
    return NO_QUEUE_EVENT;
  }

  template <typename T>
  T getLastEvent()
  {
    return (T)_lastEvent;
  }

private:
  uint8_t _lastEvent = 0;
  QueueHandle_t _queue = NULL;
  TickType_t _ticks = 10;
};

#endif