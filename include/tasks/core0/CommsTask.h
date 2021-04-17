#include <TaskBase.h>
#include <QueueManager1.h>
#include <types/SendToBoardNotf.h>
#include <types/PacketState.h>

namespace CommsTask
{
  TaskBase *thisTask;

  namespace
  {
    Queue1::Manager<SendToBoardNotf> *scheduleQueue = nullptr;
    Queue1::Manager<PacketState> *packetStateQueue = nullptr;

    PacketState state;

    bool printReplyToSchedule = false;

    void initialiseQueues()
    {
      scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(CommsTask)NotfQueue");
      packetStateQueue = Queue1::Manager<PacketState>::create("(CommsTask)PacketStateQueue");
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
      if (packetStateQueue == nullptr)
      {
        Serial.printf("ERROR: packetStateQueue is NULL\n");
        return;
      }
      packetStateQueue->reply(&state, printReplyToSchedule ? QueueBase::printSend : nullptr);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start()
  {
    thisTask = new TaskBase("CommsTask", 3000);
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