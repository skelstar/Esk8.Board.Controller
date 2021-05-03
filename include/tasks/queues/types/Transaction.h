#pragma once

#include <VescData.h>
#include <QueueBase.h>
#include <elapsedMillis.h>

class Transaction : public QueueBase
{
public:
  float version = 0.0;
  unsigned long
      packet_id,
      replyId,
      responseTime;
  bool moving = false;

public:
  Transaction() : QueueBase()
  {
    event_id = 0;
    packet_id = 0;
    name = "Transaction";
  }

  void start(ControllerData packet)
  {
    packet_id = packet.id;
  }

  void start(ControllerConfig config_packet)
  {
    packet_id = config_packet.id;
  }

  void received(VescData packet)
  {
    replyId = packet.id;
    responseTime = millis();
    version = packet.version;
    moving = packet.moving;
  }

  bool acknowledged()
  {
    return packet_id == replyId;
  }

  bool connected(unsigned long timeout)
  {
    bool online = packet_id == 0 || // ignore first packet
                  millis() - responseTime < timeout;
    if (!online)
      Serial.printf("connected(): board offline since: %lu\n",
                    millis() - responseTime);
    return online;
  }

  static void print(Transaction item, const char *preamble = nullptr)
  {
    if (preamble != nullptr)
      Serial.printf("%s: ", preamble);
    Serial.printf("event_id: %lu  ", item.event_id);
    Serial.printf("packet_id: %lu:  ", item.packet_id);
    Serial.printf("moving: %d:  ", item.moving);
    Serial.println();
  }

private:
  elapsedMillis _since_responded;
};