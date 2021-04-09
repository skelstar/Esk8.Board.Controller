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

namespace Queue1
{
  template <typename T>
  class Manager
  {
    typedef void (*QueueEventCallback)(uint16_t ev);

  public:
    elapsedMillis since_last_queue_send;
    T value;

  public:
    Manager(QueueHandle_t queue, TickType_t ticks, uint16_t noMessageValue = 0)
    {
      _queue = queue;
      _ticks = ticks;
      _noMessageValue = noMessageValue;
    }

    void sendLegacy(T *payload)
    {
      if (_queue != NULL)
      {
        xQueueSendToFront(_queue, (void *)&payload, _ticks);
        since_last_queue_send = 0;
      }
      else
      {
        Serial.printf("WARNING: Queue == NULL\n");
      }
    }

    void send(T *payload, const char *message = nullptr)
    {
      if (_queue != NULL)
      {
        xQueueSendToFront(_queue, (void *)&payload, _ticks);
        since_last_queue_send = 0;
        if (message != nullptr)
          Serial.printf("%s | event_id:%lu\n", message, payload->event_id);
        payload->event_id++;
      }
      else
      {
        Serial.printf("WARNING: Queue == NULL\n");
      }
    }

    void sendEvent(uint16_t ev)
    {
      if (_queue != NULL)
      {
        xQueueSendToBack(_queue, &ev, _ticks);
        since_last_queue_send = 0;
        if (_sentCallback != nullptr)
          _sentCallback(ev);
      }
      else
      {
        Serial.printf("WARNING: Queue == NULL\n");
      }
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

    T *peek(const char *name)
    {
      T *new_pkt = nullptr;

      if (xQueuePeek(_queue, &(new_pkt), (TickType_t)5))
      {
        // Serial.printf("%s: peeked, new packet (id: %d)\n", name, new_pkt->id);
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
    TickType_t _ticks = 10;
    QueueEventCallback
        _missedCallback = nullptr,
        _sentCallback = nullptr,
        _readCallback = nullptr;
  };
} // namespace Queue