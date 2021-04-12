#pragma once

#include <QueueManager1.h>

class SendToBoardNotf : public QueueBase
{
public:
  unsigned long event_id = 0,
                sent_time = 0,
                latency = 0;

public:
  SendToBoardNotf() : QueueBase(event_id, latency)
  {
    name = "SendToBoardNotf";
  }
};
