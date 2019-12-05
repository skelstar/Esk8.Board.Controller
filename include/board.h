#include <Arduino.h>
#include <elapsedMillis.h>

#define BOARD_COMMS_TIMEOUT 1000
#define MISSED_PACKETS_THRESHOLD  2

class Board
{
  public:

    Board() 
    {
      lastPacketRxTime = 0;
      lastPacketId = 0;
      num_missed_packets = 0;
      lost_packets = 0;
    }

    bool timed_out() {
      return millis() - lastPacketRxTime > BOARD_COMMS_TIMEOUT;
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
};