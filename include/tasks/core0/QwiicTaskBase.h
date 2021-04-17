#include <TaskBase.h>
#include <QueueManager1.h>
#include <types/SendToBoardNotf.h>

// incase we aren't using a mock in a test
#ifndef __SparkFun_Qwiic_Button_H__
#include <SparkFun_Qwiic_Button.h>
#endif

namespace QwiicTaskBase
{
  TaskBase *thisTask;

  namespace
  {
    Queue1::Manager<SendToBoardNotf> *scheduleQueue = nullptr;
    Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

    PrimaryButtonState state;

    bool printReplyToSchedule = false;

    void initialiseQueues()
    {
      scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(QwiicTaskBase)scheduleQueue");
      primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("(QwiicTaskBase)primaryButtonQueue");
    }

    void initialise()
    {
      state.correlationId = -1;
    }

    bool timeToDowork()
    {
      uint8_t status = waitForNew(scheduleQueue, PERIOD_50ms, QueueBase::printRead);
      if (status == Response::OK)
      {
        state.correlationId = scheduleQueue->payload.correlationId;
        state.sent_time = scheduleQueue->payload.sent_time;
        return true;
      }
      return false;
    }

    void doWork()
    {
      if (primaryButtonQueue == nullptr)
      {
        Serial.printf("ERROR: primaryButtonQueue is NULL\n");
        return;
      }
      primaryButtonQueue->reply(&state, printReplyToSchedule ? QueueBase::printSend : nullptr);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start()
  {
    thisTask = new TaskBase("QwiicTaskBase", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);
    thisTask->enabled = true;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }
}