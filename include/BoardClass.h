#include <VescData.h>

#define PRINT_THIS true

class BoardClass : public QueueBase
{
public:
  void save(VescData latest)
  {
    _old = packet;
    packet = latest;

    if (latest.moving)
      _changed = packet.motorCurrent != _old.motorCurrent ||
                 packet.vescOnline != _old.vescOnline;
    else
      _changed = packet.vescOnline != _old.vescOnline;
    _sinceLastPacket = 0;
    time_received = millis();
  }

  bool valuesChanged() { return _changed; }

  bool startedMoving() { return packet.moving && !_old.moving; }

  bool hasStopped() { return !packet.moving && _old.moving; }

  bool isStopped() { return !packet.moving; }

  bool isMoving() { return packet.moving; }

  bool connected()
  {
    unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
    return _sinceLastPacket <= (timeout + 200);
  }

  CommandType getCommand() { return packet.command; }

  VescData packet;
  unsigned long id;
  unsigned long sent_id;
  unsigned long time_sent = 0, time_received = 0;

private:
  elapsedMillis _sinceLastPacket;
  VescData _old;
  bool _changed = false;
};

class PacketState : public QueueBase
{
public:
  float version = 0.0;

  void sent(ControllerPacketBase packet)
  {
    _since_sent = 0;
    _sent_packet_id = packet.id;
  }

  void sent(ControllerConfig config_packet)
  {
    _since_sent = 0;
    _sent_packet_id = config_packet.id;
  }

  void received(VescData packet)
  {
    _reply_packet_id = packet.id;
    version = packet.version;
  }

  bool acknowledged()
  {
    return _sent_packet_id == _reply_packet_id;
  }

  bool connected()
  {
    // if (_sent_packet_id == 0 || // ignore first packet
    //     _sent_packet_id == _reply_packet_id ||
    //     _since_sent < 200)
    // {
    //   Serial.printf("_sent_packet_id == 0 %d  \n", _sent_packet_id == 0);
    //   Serial.printf("_sent_packet_id == _reply_packet_id %d\n", _sent_packet_id == _reply_packet_id);
    //   Serial.printf("_since_sent < 200 %d\n", _since_sent < 200);
    // }
    return _sent_packet_id == 0 || // ignore first packet
           _sent_packet_id == _reply_packet_id ||
           _since_sent < 200;
  }

private:
  elapsedMillis _since_sent;
  unsigned long
      _sent_packet_id,
      _reply_packet_id;
};
