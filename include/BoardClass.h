class BoardClass
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
    sinceLastPacket = 0;
  }

  bool valuesChanged() { return _changed; }

  bool startedMoving() { return packet.moving && !_old.moving; }

  bool hasStopped() { return !packet.moving && _old.moving; }

  bool isStopped() { return !packet.moving; }

  bool isMoving() { return packet.moving; }

  bool hasTimedout()
  {
    unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
    return sinceLastPacket > (timeout + 100);
  }

  CommandType getCommand() { return packet.command; }

  VescData packet;
  elapsedMillis sinceLastPacket;

private:
  VescData _old;
  bool _changed = false;
};
