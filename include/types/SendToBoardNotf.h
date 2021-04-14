#pragma once

#include <QueueManager1.h>

class SendToBoardNotf : public QueueBase
{
public:
public:
  SendToBoardNotf() : QueueBase()
  {
    name = "SendNotf";
  }
};
