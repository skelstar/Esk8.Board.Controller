#pragma once

const TickType_t TICKS_5ms = 5 / portTICK_PERIOD_MS;
const TickType_t TICKS_10ms = 10 / portTICK_PERIOD_MS;
const TickType_t TICKS_50ms = 50 / portTICK_PERIOD_MS;
const TickType_t TICKS_100ms = 100 / portTICK_PERIOD_MS;
const TickType_t TICKS_500ms = 500 / portTICK_PERIOD_MS;

#include <types/QueueType.h>

class QueueBase
{
protected:
  unsigned long _event_id = 0, _latency = 0;

public:
  const char *name = nullptr;
  uint8_t queue_type = QueueType::QT_NONE;

public:
  QueueBase(unsigned long event_id, unsigned long latency)
  {
    name = "not set";
    _event_id = event_id;
    _latency = latency;
  }

  bool been_peeked(unsigned long prev_id)
  {
    return _event_id == prev_id;
  }

  bool missed_packet(unsigned long prev_id)
  {
    return prev_id == 0 && _event_id - prev_id > 1;
  }

  static void printSend(QueueBase b)
  {
    Serial.printf("[Queue|Send|%s|%lums] id: %lu\n",
                  getQueueType(b.queue_type),
                  millis(),
                  b._event_id);
  }

  static void printRead(QueueBase b)
  {
    Serial.printf("[Queue|Read|%s|%lums] id: %lu\n",
                  getQueueType(b.queue_type),
                  millis(),
                  b._event_id);
  }
};
