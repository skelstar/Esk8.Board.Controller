#include <VescData.h>

class BoardClass : public QueueBase
{
public:
  VescData packet;
  unsigned long id,
      sent_id,
      time_sent = 0,
      time_received = 0,
      event_id = 0,
      latency = 0;

public:
  BoardClass() : QueueBase(event_id, latency)
  {
    event_id = 0;
    name = "BoardClass";
  }

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

private:
  elapsedMillis _sinceLastPacket;
  VescData _old;
  bool _changed = false;
};
