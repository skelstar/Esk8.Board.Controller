#pragma once

#include <QueueManager.h>

class PacketState : public QueueBase
{
public:
  float version = 0.0;
  unsigned long packet_id;

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
    version = packet.version;
    _moving = packet.moving;
  }

  bool isMoving()
  {
    return _moving;
  }

  bool acknowledged()
  {
    return packet_id == _reply_packet_id;
  }

  bool connected()
  {
    return packet_id == 0 || // ignore first packet
           packet_id == _reply_packet_id ||
           _since_sent < 200;
  }

private:
  elapsedMillis _since_sent;
  unsigned long _reply_packet_id;
  bool _moving = false;
};