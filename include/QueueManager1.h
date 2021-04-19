#pragma once

#define ESK8_QUEUE_MANAGER1

#ifndef ARDUINO_H
#include "Arduino.h"
#endif

#include <types/PacketState.h>
#include <types/SendToBoardNotf.h>
#include <types/NintendoButtonEvent.h>
#include <types/PrimaryButton.h>
#include <types/Throttle.h>
#include <types/QueueBase.h>
#include <types/DisplayEvent.h>

const unsigned long SECONDS = 1000;
const unsigned long MILLIS_S = 1;

const unsigned long PERIOD_10ms = 10;
const unsigned long PERIOD_20ms = 20;
const unsigned long PERIOD_30ms = 30;
const unsigned long PERIOD_40ms = 40;
const unsigned long PERIOD_50ms = 50;
const unsigned long PERIOD_100ms = 100;
const unsigned long PERIOD_200ms = 200;
const unsigned long PERIOD_300ms = 300;
const unsigned long PERIOD_500ms = 500;
const unsigned long PERIOD_1S = 1000;
const unsigned long PERIOD_2S = 2000;

#define PRINT_TIMEOUT 1

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
  template <typename T>
  class Manager
  {
    typedef void (*QueueEventCallback)(uint16_t ev);
    typedef void (*SentCallback)(T packet);
    typedef void (*SentCallback_r)(QueueBase packet, const char *queueName);

  public:
    T payload;

  public:
    static Manager *create(const char *name, TickType_t ticks = TICKS_5ms)
    {
      if (std::is_same<T, SendToBoardNotf>::value)
      {
        return new Manager<T>(xSendToBoardQueueHandle, TICKS_5ms, name);
      }
      if (std::is_same<T, PrimaryButtonState>::value)
      {
        return new Manager<T>(xPrimaryButtonQueueHandle, TICKS_5ms, name);
      }
      if (std::is_same<T, ThrottleState>::value)
      {
        return new Manager<T>(xThrottleQueueHandle, TICKS_5ms, name);
      }
      if (std::is_same<T, PacketState>::value)
      {
        return new Manager<T>(xPacketStateQueueHandle, TICKS_5ms, name);
      }
      if (std::is_same<T, NintendoButtonEvent>::value)
      {
        return new Manager<T>(xNintendoControllerQueue, TICKS_5ms, name);
      }
      if (std::is_same<T, DisplayEvent>::value)
      {
        return new Manager<T>(xDisplayQueueHandle, TICKS_5ms, name);
      }
      Serial.printf("ERROR: (Manager::create) a queue has not been created for this type (%s)\n", name);
      return nullptr;
    }

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
    }

    void sendLegacy(T *payload)
    {
      xQueueSendToFront(_queue, (void *)&payload, _ticks);
    }

    void send(T *payload, SentCallback sent_cb = nullptr)
    {
      Serial.printf("---------------------------\n   WARNING: this is deprecated!!!\n---------------------------\n");
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
      payload->correlationId++;
      if (_queue == nullptr)
      {
        Serial.printf("ERROR: queue not initialised! (%s)\n", name);
        return;
      }
      xQueueSendToFront(_queue, (void *)&payload, _ticks);

      vTaskDelay(1); // think there was a bug where it was incrementing before it was sent
      if (sent_cb != nullptr)
        sent_cb(*payload, name);

      payload->sent_time = millis();
      payload->event_id++;
    }

    void send_n(QueueBase *payload, SentCallback_r sent_cb = nullptr)
    {
      if (_queue == nullptr)
      {
        Serial.printf("ERROR: queue not initialised! (%s)\n", name);
        return;
      }
      xQueueSendToFront(_queue, (void *)&payload, _ticks);

      if (sent_cb != nullptr)
        sent_cb(*payload, name);

      payload->sent_time = millis();
      payload->event_id++;
      payload->correlationId++;
    }

    void reply(QueueBase *payload, SentCallback_r sent_cb = nullptr)
    {
      if (_queue == nullptr)
      {
        Serial.printf("ERROR: queue not initialised! (%s)\n", name);
        return;
      }
      xQueueSendToFront(_queue, (void *)&payload, _ticks);

      if (sent_cb != nullptr)
        sent_cb(*payload, name);

      payload->sent_time = millis();
      payload->event_id++;
    }

    // this will clear the queue
    T read()
    {
      T e;
      if (_queue != NULL && xQueueReceive(_queue, &e, _ticks) == pdPASS)
      {
        // if (_readCallback != nullptr)
        //   _readCallback(e);
        return (T)e;
      }
      return (T)e;
    }

    bool hasValue()
    {
      T *result = this->peek();
      if (result != nullptr && ((QueueBase *)result)->correlationId != _last_event_id)
      {
        // _checkForMissedEvents(((QueueBase *)result)->correlationId);
        _last_event_id = ((QueueBase *)result)->correlationId;
        payload = *result;
        return true;
      }
      return false;
    }

    T *peek()
    {
      if (_queue == nullptr)
      {
        DEBUG("ERROR (peek): _queue is NULL");
        return nullptr;
      }
      T *new_pkt = nullptr;
      xQueuePeek(_queue, &(new_pkt), _ticks);
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
    void _checkForMissedEvents(unsigned long p_id)
    {
      uint16_t missed_packet_count = p_id - _last_event_id - 1;
      if (_missedCallback != nullptr &&
          missed_packet_count > 0 &&
          _last_event_id > 0)
        _missedCallback(missed_packet_count);
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
  };

} // namespace Queue

bool take(SemaphoreHandle_t m_handle, TickType_t ticks = TICKS_5ms)
{
  if (m_handle != nullptr)
    return (xSemaphoreTake(m_handle, (TickType_t)5) == pdPASS);
  return false;
}

void give(SemaphoreHandle_t m_handle)
{
  if (m_handle != nullptr)
    xSemaphoreGive(m_handle);
}

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
