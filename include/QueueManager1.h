#pragma once

#define ESK8_QUEUE_MANAGER1

#ifndef ARDUINO_H
#include "Arduino.h"
#endif

#include <types/QueueBase.h>
#include <types/Throttle.h>

template <typename T>
void printSentToQueue(T payload)
{
  if (std::is_base_of<QueueBase, T>::value)
    Serial.printf("[%s] sent: id:%lu to queue\n", payload.name != nullptr ? payload.name : "", payload.event_id);
}

namespace Queue1
{
  template <typename T>
  class Manager
  {
    typedef void (*QueueEventCallback)(uint16_t ev);
    typedef void (*SentCallback)(T packet);

  public:
    T value;

  public:
    Manager(QueueHandle_t queue, TickType_t ticks, const char *name = nullptr)
    {
      if (queue == nullptr)
      {
        Serial.printf("ERROR: queue is null (Queue::Manager constr)\n");
        return;
      }
      _queue = queue;
      _ticks = ticks;
      _queue_name = name != nullptr ? name : ((QueueBase)value).name;
    }

    void sendLegacy(T *payload)
    {
      xQueueSendToFront(_queue, (void *)&payload, _ticks);
    }

    void send(T *payload, SentCallback sent_cb = nullptr)
    {
      xQueueSendToFront(_queue, (void *)&payload, _ticks);

      if (_queue_name != nullptr)
        Serial.printf("[%s] send id: %lu\n", _queue_name, payload->event_id);

      if (sent_cb != nullptr)
        sent_cb(*payload);

      payload->event_id++;
    }

    // this will clear the queue
    T read()
    {
      T e = nullptr;
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
      if (xQueuePeek(_queue, &(new_pkt), _ticks) &&
          new_pkt->event_id != _last_event_id &&
          name != nullptr)
        Serial.printf("%s: peeked, new packet (id: %lu)\n", name, new_pkt->event_id);
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
    uint16_t _lastEvent = 0;
    unsigned long _last_event_id = -1;
    QueueHandle_t _queue = NULL;
    const char *_queue_name = nullptr;
    TickType_t _ticks = 10;
    QueueEventCallback
        _missedCallback = nullptr,
        _sentCallback = nullptr,
        _readCallback = nullptr;
  };
} // namespace Queue