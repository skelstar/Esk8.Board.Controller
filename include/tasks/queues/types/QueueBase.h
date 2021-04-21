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

class QueueBase
{
public:
  enum Command
  {
    NONE = 0,
    RESPOND,
  };

  unsigned long event_id = 0, latency = 0;
  const char *name = nullptr;
  unsigned long sent_time;
  Command command = NONE;

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
    Serial.printf("[Queue|%s| SEND |%lums] event_id: %lu %s\n",
                  queueName != nullptr ? queueName : b.name,
                  millis(),
                  b.event_id,
                  b.command == RESPOND ? "RESPOND" : "");
  }

  static void printReply(QueueBase b, const char *queueName = nullptr)
  {
    Serial.printf("[Queue|%s| REPLY |%lums] event_id: %lu %s (took %lums)\n",
                  queueName != nullptr ? queueName : b.name,
                  millis(),
                  b.event_id,
                  b.command == RESPOND ? "RESPONSE" : "",
                  b.getSinceSent());
  }

  static void printRead(QueueBase b, const char *queueName = nullptr)
  {
    Serial.printf("[Queue|%s| READ |%lums] event_id: %lu after %lums %s\n",
                  queueName != nullptr ? queueName : b.name,
                  millis(),
                  b.event_id,
                  b.getSinceSent(),
                  b.command == RESPOND ? "RESPOND" : "");
  }
};