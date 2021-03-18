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
    _sinceLastPacket = 0;
  }

  bool valuesChanged() { return _changed; }

  bool startedMoving() { return packet.moving && !_old.moving; }

  bool hasStopped() { return !packet.moving && _old.moving; }

  bool isStopped() { return !packet.moving; }

  bool isMoving() { return packet.moving; }

  bool connected()
  {
    unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
    bool connected = _sinceLastPacket <= (timeout + 200);
    if (!connected)
      DEBUGVAL(_sinceLastPacket);
    return connected;
    // return _sinceLastPacket <= (timeout + 200);
  }

  CommandType getCommand() { return packet.command; }

  VescData packet;
  unsigned long id;

private:
  elapsedMillis _sinceLastPacket;
  VescData _old;
  bool _changed = false;
};
