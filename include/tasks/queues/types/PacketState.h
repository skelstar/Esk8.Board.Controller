#pragma once

#include <VescData.h>
#include <tasks/queues/types/QueueBase.h>
#include <elapsedMillis.h>

class PacketState : public QueueBase
{
public:
  float version = 0.0;
  unsigned long packet_id;
  bool moving = false;

public:
  PacketState() : QueueBase()
  {
    event_id = 0;
    name = "PacketState";
  }

  void sent(ControllerPacketBase packet)
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
    _since_responded = 0;
    version = packet.version;
    moving = packet.moving;
  }

  bool acknowledged()
  {
    return packet_id == _reply_packet_id;
  }

  bool connected()
  {
    return packet_id == 0 || // ignore first packet
           packet_id == _reply_packet_id ||
           _since_responded < 200;
  }

  static void print(PacketState item, const char *preamble = nullptr)
  {
    if (preamble != nullptr)
      Serial.printf("%s: ", preamble);
    Serial.printf("event_id: %lu  ", item.event_id);
    Serial.printf("packet_id: %lu:  ", item.packet_id);
    Serial.printf("moving: %d:  ", item.moving);
    Serial.println();
  }

private:
  elapsedMillis _since_sent, _since_responded;
  unsigned long _reply_packet_id;
};