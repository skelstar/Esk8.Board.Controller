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
      roundTripTime; // time it took for the board to reply
  bool moving = false;

public:
  BoardState() : QueueBase()
  {
    event_id = 0;
    packet_id = 0;
    name = "BoardState";
  }

  void sent(ControllerData packet)
  {
    _since_sent = 0;
    packet_id = packet.id;
  }

  void sent(ControllerConfig config_packet)
  {
    _since_sent = 0;
    packet_id = config_packet.id;
  }

  void received(VescData packet)
  {
    _reply_packet_id = packet.id;
    roundTripTime = millis() - packet.txTime;
    version = packet.version;
    moving = packet.moving;
  }

  bool acknowledged()
  {
    return packet_id == _reply_packet_id;
  }

  bool connected(unsigned long responseWindow)
  {
    return packet_id == 0 || // ignore first packet
           (packet_id == _reply_packet_id &&
            roundTripTime < responseWindow);
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
  elapsedMillis _since_sent, _since_responded;
  unsigned long
      _reply_packet_id;
};