#pragma once

#include <QueueManager1.h>

class SendToBoardNotf : public QueueBase
{
public:
  const static int NO_CORRELATION = 0;

public:
  SendToBoardNotf() : QueueBase()
  {
    name = "SendNotf";
  }
};
