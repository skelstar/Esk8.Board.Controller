#pragma once

#define ESK8_QUEUE_MANAGER1

#ifndef ARDUINO_H
#include "Arduino.h"
#endif

#include <types/QueueBase.h>
#include <types/Throttle.h>

const unsigned long SECONDS = 1000;
const unsigned long MILLIS_S = 1;

const unsigned long PERIOD_10ms = 10;
const unsigned long PERIOD_20ms = 20;
const unsigned long PERIOD_30ms = 30;
const unsigned long PERIOD_40ms = 40;
const unsigned long PERIOD_50ms = 50;
const unsigned long PERIOD_100ms = 100;
const unsigned long PERIOD_200ms = 200;
const unsigned long PERIOD_500ms = 500;
const unsigned long PERIOD_1S = 1000;
const unsigned long PERIOD_2S = 2000;

typedef void (*ResponseCallback1)(QueueBase packet, const char *queueName);

namespace Response
{
  enum WaitResp
  {
    OK = 0,
    TIMEOUT,
  };
};

namespace Queue1
{
  const int HISTORY_LENGTH = 3;

  template <typename T>
  class Manager
  {
    typedef void (*QueueEventCallback)(uint16_t ev);
    typedef void (*SentCallback)(T packet);
    typedef void (*SentCallback_r)(QueueBase packet, const char *queueName);

  public:
    T payload;

  public:
    Manager(QueueHandle_t queue, TickType_t ticks, const char *p_name = nullptr)
    {
      if (queue == nullptr)
      {
        Serial.printf("ERROR: queue is null (Queue::Manager constr)\n");
        return;
      }
      _queue = queue;
      _ticks = ticks;
      name = p_name != nullptr ? p_name : ((QueueBase)payload).name;

      _initHistory();
    }

    void sendLegacy(T *payload)
    {
      xQueueSendToFront(_queue, (void *)&payload, _ticks);
    }

    void send(T *payload, SentCallback sent_cb = nullptr)
    {
      if (_queue == nullptr)
      {
        Serial.printf("ERROR: queue not initialised! (%s)\n", name);
        return;
      }
      xQueueSendToFront(_queue, (void *)&payload, _ticks);

      if (sent_cb != nullptr)
        sent_cb(*payload);

      payload->event_id++;
    }

    void send_r(T *payload, SentCallback_r sent_cb = nullptr)
    {
      if (_queue == nullptr)
      {
        Serial.printf("ERROR: queue not initialised! (%s)\n", name);
        return;
      }
      xQueueSendToFront(_queue, (void *)&payload, _ticks);

      if (sent_cb != nullptr)
        sent_cb(*payload, name);

      _addToHistory(*payload);

      payload->sent_time = millis();
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
        payload = *result;
        return true;
      }
      return false;
    }

    T *peek(const char *name = nullptr)
    {
      // TODO check for null queue
      T *new_pkt = nullptr;
      xQueuePeek(_queue, &(new_pkt), _ticks);
      return new_pkt;
    }

    T getFromHistory(uint8_t offset)
    {
      if (offset < HISTORY_LENGTH)
        return _getFromHistory(offset);
      DEBUG("ERROR: going back too far in history!");
      return _getFromHistory(0);
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

    void _initHistory()
    {
      for (int i = 0; i < HISTORY_LENGTH; i++)
        _history[i] = nullptr;
    }

    void _addToHistory(T item)
    {
      int _old_idx = _historyIdx;
      _history[_historyIdx] = new T(item);
      _historyIdx = (_historyIdx < HISTORY_LENGTH - 1) ? _historyIdx + 1 : 0;
      // DEBUGMVAL("_addToHistory", _old_idx, _historyIdx, _history[_old_idx]->event_id);
    }

    T _getFromHistory(int stepsBack)
    {
      int calc = _historyIdx - stepsBack;
      if (calc < 0)
        calc = HISTORY_LENGTH - calc;
      T *result = new T(*_history[calc]);
      // DEBUGMVAL("_getFromHistory", stepsBack, _historyIdx, calc, result->event_id);
      return *result;
    }

  public:
    const char *name = "Queue name not supplied";

  private:
    unsigned long _last_event_id;
    QueueHandle_t _queue = NULL;
    TickType_t _ticks = 10;
    QueueEventCallback
        _missedCallback = nullptr,
        _sentCallback = nullptr,
        _readCallback = nullptr;

    T *_history[HISTORY_LENGTH];
    uint8_t _historyIdx = 0;
  };
} // namespace Queue

template <typename T>
Response::WaitResp waitForNew(Queue1::Manager<T> *queue,
                              uint16_t timeout,
                              ResponseCallback1 gotResponse1_cb = nullptr,
                              bool printTimeout = false)
{
  elapsedMillis since_started_listening = 0;
  do
  {
    if (queue->hasValue())
    {
      if (gotResponse1_cb != nullptr)
        gotResponse1_cb(queue->payload, queue->name);

      return Response::OK;
    }
    vTaskDelay(1);
  } while (since_started_listening < timeout);

  if (printTimeout)
    DEBUGMVAL("timeout: ", queue->name, timeout);
  return Response::TIMEOUT;
}
