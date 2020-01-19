#ifndef StatsLib_h
#define StatsLib_h

#include <Arduino.h>

class StatsLib
{
public:
  bool first_packet_updated;
  bool request_delay_updated;
  bool force_update;
  uint8_t retry_rate = 0;

  bool changed()
  {
    bool changed1 = first_packet_updated || request_delay_updated || force_update;
    first_packet_updated = false;
    request_delay_updated = false;
    force_update = true;
    return changed1;
  }
};

#endif