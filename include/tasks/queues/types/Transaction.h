#pragma once

#include <VescData.h>
#include <QueueBase.h>
#include <elapsedMillis.h>

class BoardState : public QueueBase
{
public:
  float version = 0.0;
  unsigned long
      packet_id,
      replyId,
      startedTime,
      responseTime,
      roundTripTime; // time it took for the board to reply
  bool moving = false;

public:
  BoardState() : QueueBase()
  {
    event_id = 0;
    packet_id = 0;
    name = "BoardState";
  }

  void start(ControllerData packet)
  {
    packet_id = packet.id;
    startedTime = millis();
  }

  void start(ControllerConfig config_packet)
  {
    packet_id = config_packet.id;
    startedTime = millis();
  }

  void received(VescData packet)
  {
    replyId = packet.id;
    roundTripTime = millis() - packet.txTime;
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

  static void print(BoardState item, const char *preamble = nullptr)
  {
    if (preamble != nullptr)
      Serial.printf("%s: ", preamble);
    Serial.printf("event_id: %lu  ", item.event_id);
    Serial.printf("packet_id: %lu:  ", item.packet_id);
    Serial.printf("moving: %d:  ", item.moving);
    Serial.printf("round trip: %lu:  ", item.roundTripTime);
    Serial.println();
  }

private:
  elapsedMillis _since_responded;
};