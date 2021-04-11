#pragma once

#include <QueueManager1.h>

class SendToBoardNotf : public QueueBase
{
public:
  SendToBoardNotf() : QueueBase(event_id)
  {
    name = "SendToBoardNotf";
  }

  unsigned long event_id = 0,
                sent_time = 0;
};
