#include <TaskBase.h>
#include <QueueManager1.h>
#include <types/SendToBoardNotf.h>
#include <types/PrimaryButton.h>

namespace BaseTaskTest1
{
  TaskBase *thisTask;

  namespace
  {
    Queue1::Manager<SendToBoardNotf> *notfQueue = nullptr;
    Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

    PrimaryButtonState state;

    void initialiseQueues()
    {
      notfQueue = Queue1::Manager<SendToBoardNotf>::create("(BaseTaskTest)NotfQueue");
      primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("(BaseTaskTest)primaryButtonQueue");
    }

    void initialise()
    {
    }

    bool timeToDowork()
    {
      if (notfQueue->hasValue())
      {
        // DEBUGMVAL("timeToDOwOrk", notfQueue->payload.correlationId, notfQueue->payload.sent_time);
        state.correlationId = notfQueue->payload.correlationId;
        state.sent_time = notfQueue->payload.sent_time;
        // state.name = notfQueue->payload.name;
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
      state.correlationId = notfQueue->payload.correlationId;
      primaryButtonQueue->send_r(&state);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start()
  {
    thisTask = new TaskBase("BaseTaskTest1", 3000);
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