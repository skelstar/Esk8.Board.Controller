#include <Arduino.h>

class Board
{
  public:
    typedef enum BoardEventEnum
    {
      EV_BOARD_ONLINE = 0,
      EV_BOARD_TIMEOUT,
    };

    typedef void (*eventCallBack)(BoardEventEnum ev);

    // #define CHECK_FOR_BOARD_TIMEOUT 1

    Board()
    {
      lastPacketRxTime = 0;
      lastPacketId = 0;
      total_missed_packets = 0;
      lost_packets = 0;
    }

    void setOnEvent(eventCallBack ptr)
    {
      _on_event_callback = ptr;
    }

    bool timed_out()
    {
      #ifdef CHECK_FOR_BOARD_TIMEOUT
      return millis() - lastPacketRxTime > BOARD_COMMS_TIMEOUT;
      #else
      return false;
      #endif
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

    uint8_t received_packet(unsigned long rx_id)
    {
      lastPacketRxTime = millis();
    }

    unsigned long lost_packets;
    unsigned long lastPacketRxTime;
    unsigned long lastPacketId;
    unsigned long total_missed_packets = 0;

  private:
    eventCallBack _on_event_callback;
};