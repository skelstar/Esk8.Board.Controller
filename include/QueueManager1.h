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
  // typename std::enable_if<std::is_base_of<QueueBase, T>::value, void>::type
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

    // template <typename T>
    // typename std::enable_if<std::is_base_of<QueueBase, T>::value, void>::type
    // increments the T->id
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

    bool messageAvailable()
    {
      uint16_t *peeked_val;
      return _queue != NULL && xQueuePeek(_queue, &peeked_val, _ticks) == pdTRUE;
    }

    // template <typename T>
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

    // template <typename T>
    bool hasValue(const char *name = nullptr)
    {
      T *result = this->peek(name);
      if (result != nullptr && result->event_id != _last_event_id)
      {
        _last_event_id = result->event_id;
        value = *result;
        return true;
      }
      return false;
    }

    // template <typename T>
    T *peek(const char *name)
    {
      T *new_pkt = nullptr;

      if (xQueuePeek(_queue, &(new_pkt), (TickType_t)5))
      {
        // Serial.printf("%s: peeked, new packet (id: %d)\n", name, new_pkt->id);
      }
      return new_pkt;
    }

    // template <typename T>
    T getLastEvent()
    {
      return (T)_lastEvent;
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
    // const char *_queueName = nullptr;
    uint16_t _lastEvent = 0, _noMessageValue;
    unsigned long _last_event_id = -1;
    QueueHandle_t _queue = NULL;
    TickType_t _ticks = 10;
    QueueEventCallback
        _sentCallback = nullptr,
        _readCallback = nullptr;
  };
} // namespace Queue