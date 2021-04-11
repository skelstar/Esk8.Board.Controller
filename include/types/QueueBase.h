#pragma once

#define TICKS_5ms 5 / portTICK_PERIOD_MS
#define TICKS_10ms 10 / portTICK_PERIOD_MS
#define TICKS_50ms 50 / portTICK_PERIOD_MS
#define TICKS_100ms 100 / portTICK_PERIOD_MS

class QueueBase
{
private:
  unsigned long _event_id = 0;

public:
  const char *name = nullptr;

public:
  QueueBase(unsigned long event_id)
  {
    _event_id = event_id;
  }

  bool been_peeked(unsigned long prev_id)
  {
    return _event_id == prev_id;
  }

  bool missed_packet(unsigned long prev_id)
  {
    return prev_id == 0 && _event_id - prev_id > 1;
  }
};
