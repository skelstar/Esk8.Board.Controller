#pragma once

#define ESK8_QUEUE_MANAGER1

#ifndef ARDUINO_H
#include "Arduino.h"
#endif

#ifndef QUEUEBASE_CLASS
#define QUEUEBASE_CLASS

class QueueBase
{
public:
  unsigned long event_id;

  bool been_peeked(unsigned long prev_id)
  {
    return event_id == prev_id;
  }

  bool missed_packet(unsigned long prev_id)
  {
    return prev_id == 0 && event_id - prev_id > 1;
  }
};
#endif

#include <types/Throttle.h>

namespace Queue1
{
  template <typename T>
  class Manager
  {
    typedef void (*QueueEventCallback)(uint16_t ev);
    typedef void (*VoidCallback)(T packet);

  public:
    elapsedMillis since_last_queue_send;
    T value;

  public:
    Manager(QueueHandle_t queue, TickType_t ticks, uint16_t noMessageValue = 0)
    {
      if (queue == nullptr)
      {
        Serial.printf("ERROR: queue is null (Queue::Manager constr)\n");
        return;
      }
      _queue = queue;
      _ticks = ticks;
      _noMessageValue = noMessageValue;
    }

    // Manager(QueueHandle_t queue, const char *queueName, TickType_t ticks, uint16_t noMessageValue = 0)
    // {
    //   if (queue == nullptr)
    //   {
    //     Serial.printf("ERROR: queue is null (Queue::Manager constr)\n");
    //     return;
    //   }
    //   _queue = queue;
    //   _queue_name = queueName;
    //   _ticks = ticks;
    //   _noMessageValue = noMessageValue;
    // }

    // Manager(QueueHandle_t queue, TickType_t ticks, uint16_t noMessageValue = 0)
    // {
    //   Manager(queue, nullptr, ticks, noMessageValue);
    // }

    void sendLegacy(T *payload)
    {
      xQueueSendToFront(_queue, (void *)&payload, _ticks);
      since_last_queue_send = 0;
    }

    void send(T *payload, VoidCallback sent_cb = nullptr)
    {
      xQueueSendToFront(_queue, (void *)&payload, _ticks);
      since_last_queue_send = 0;

      if (sent_cb != nullptr)
      {
        sent_cb(*payload);
      }

      // if (_sentCallback != nullptr)
      //   _sentCallback(payload->event_id);

      payload->event_id++;
    }

    // this will clear the queue
    T read()
    {
      uint16_t e = _noMessageValue;
      if (_queue != NULL && xQueueReceive(_queue, &e, _ticks) == pdPASS)
      {
        if (_readCallback != nullptr)
          _readCallback(e);
        return (T)e;
      }
      return (T)e;
    }

    bool hasValue(const char *name = nullptr)
    {
      T *result = this->peek(name);
      if (result != nullptr && result->event_id != _last_event_id)
      {
        _checkForMissedEvents(result->event_id);
        _last_event_id = result->event_id;
        value = *result;
        return true;
      }
      return false;
    }

    T *peek(const char *name = nullptr)
    {
      T *new_pkt = nullptr;

      if (xQueuePeek(_queue, &(new_pkt), (TickType_t)5) && name != nullptr)
      {
        Serial.printf("%s: peeked, new packet (id: %d)\n", name, new_pkt->event_id);
      }
      return new_pkt;
    }

    bool missedPacket()
    {
      return QueueBase::missed_packet(_last_event_id);
    }

    void setMissedEventCallback(QueueEventCallback cb)
    {
      _missedCallback = cb;
    }

    void setSentEventCallback(QueueEventCallback cb)
    {
      _sentCallback = cb;
    }

    void setReadEventCallback(QueueEventCallback cb)
    {
      _readCallback = cb;
    }

  private:
    void _checkForMissedEvents(unsigned long event_id)
    {
      uint16_t missed_packet_count = event_id - _last_event_id - 1;
      if (_missedCallback != nullptr &&
          missed_packet_count > 0 &&
          _last_event_id > 0)
        _missedCallback(missed_packet_count);
    }

  private:
    uint16_t _lastEvent = 0, _noMessageValue;
    long _last_event_id = -1;
    QueueHandle_t _queue = NULL;
    const char *_queue_name = nullptr;
    TickType_t _ticks = 10;
    QueueEventCallback
        _missedCallback = nullptr,
        _sentCallback = nullptr,
        _readCallback = nullptr;
  };
} // namespace Queue