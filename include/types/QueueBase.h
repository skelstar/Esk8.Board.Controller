#pragma once

const TickType_t TICKS_5ms = 5 / portTICK_PERIOD_MS;
const TickType_t TICKS_10ms = 10 / portTICK_PERIOD_MS;
const TickType_t TICKS_50ms = 50 / portTICK_PERIOD_MS;
const TickType_t TICKS_100ms = 100 / portTICK_PERIOD_MS;
const TickType_t TICKS_500ms = 500 / portTICK_PERIOD_MS;
const TickType_t TICKS_1s = 1000 / portTICK_PERIOD_MS;
const TickType_t TICKS_2s = 2000 / portTICK_PERIOD_MS;
const TickType_t TICKS_3s = 3000 / portTICK_PERIOD_MS;
const TickType_t TICKS_4s = 4000 / portTICK_PERIOD_MS;

#include <types/QueueType.h>

class QueueBase
{
public:
  unsigned long event_id = 0, latency = 0, correlationId = 0;
  const char *name = nullptr;
  unsigned long sent_time;

public:
  QueueBase()
  {
    event_id = 0;
    name = "not set";
  }

  bool been_peeked(unsigned long prev_id)
  {
    return event_id == prev_id;
  }

  bool missed_packet(unsigned long prev_id)
  {
    return prev_id == 0 && event_id - prev_id > 1;
  }

  unsigned long getSinceSent()
  {
    return millis() - sent_time;
  }

  static void printSend(QueueBase b, const char *queueName = nullptr)
  {
    Serial.printf("[Queue|%s| --> |%lums] correlationID: %lu\n",
                  queueName != nullptr ? queueName : b.name,
                  millis(),
                  b.correlationId);
  }

  static void printRead(QueueBase b, const char *queueName = nullptr)
  {
    Serial.printf("[Queue|%s| <-- |%lums] correlationId: %lu after %lums\n",
                  queueName != nullptr ? queueName : b.name,
                  millis(),
                  b.correlationId,
                  b.getSinceSent());
  }
};
