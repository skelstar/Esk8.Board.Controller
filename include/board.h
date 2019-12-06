#include <Arduino.h>

class Board
{
public:
  typedef enum BoardEventEnum
  {
    EV_BOARD_ONLINE = 0,
    EV_BOARD_TIMEOUT,
    EV_BOARD_MISSED_PACKETS,
  };

  typedef void (*eventCallBack)(BoardEventEnum ev);

  Board()
  {
    lastPacketRxTime = 0;
    lastPacketId = 0;
    num_missed_packets = 0;
    lost_packets = 0;
  }

  void setOnEvent(eventCallBack ptr)
  {
    _on_event_callback = ptr;
  }

  bool timed_out()
  {
    return millis() - lastPacketRxTime > BOARD_COMMS_TIMEOUT;
  }

  void loop()
  {
    if (timed_out())
    {
      _on_event_callback(EV_BOARD_TIMEOUT);
    }
    else 
    {
      _on_event_callback(EV_BOARD_ONLINE);
    }
  }

  uint8_t update(unsigned long rx_id)
  {
    lost_packets = 0;
    missed_packets = false;
    int8_t diff = rx_id - 1 - lastPacketId;

    if (diff >= MISSED_PACKETS_THRESHOLD)
    {
      lost_packets = rx_id - MISSED_PACKETS_THRESHOLD - lastPacketId;
      num_missed_packets += lost_packets;
      missed_packets = true;
    }
    lastPacketId = rx_id;
  }

  unsigned long num_missed_packets;
  unsigned long lost_packets;
  unsigned long lastPacketRxTime;
  unsigned long lastPacketId;

  bool missed_packets = false;

private:
  eventCallBack _on_event_callback;
};