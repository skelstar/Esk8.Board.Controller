#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

#define HAPTIC_TASK

class HapticTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;
  Queue1::Manager<Transaction> *transactionQueue = nullptr;

  SimplMessageObj simplMessage;
  Transaction transaction;

public:
  HapticTask() : TaskBase("HapticTask", 3000, PERIOD_50ms)
  {
    _core = CORE_0;
  }

  void _initialise()
  {
    simplMsgQueue = createQueueManager<SimplMessageObj>("(HapticTask)simplMsgQueue");

    transactionQueue = createQueueManager<Transaction>("(HapticTask)transactionQueue");
  }

  void doWork()
  {
    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    if (transactionQueue->hasValue())
      _handleTransaction(transactionQueue->payload);
  }

  void cleanup()
  {
    delete (simplMsgQueue);
    delete (transactionQueue);
  }

  void _handleSimplMessage(SimplMessageObj simplMessage)
  {
  }

  void _handleTransaction(Transaction transaction)
  {
  }
};

HapticTask hapticTask;

namespace nsHapticTask
{
  void task1(void *parameters)
  {
    hapticTask.task(parameters);
  }
}
