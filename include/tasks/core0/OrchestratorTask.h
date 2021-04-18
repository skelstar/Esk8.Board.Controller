#include <TaskBase.h>
#include <QueueManager1.h>
#include <types/SendToBoardNotf.h>
#include <types/PrimaryButton.h>

namespace OrchestratorTask
{
  TaskBase *thisTask;

  namespace
  {
    Queue1::Manager<SendToBoardNotf> *orchestratorQueue = nullptr;

    elapsedMillis since_last_notification;

    unsigned long sendInterval = PERIOD_500ms;
    bool printSendToQueue = false;

    SendToBoardNotf notification;

    void initialiseQueues()
    {
      orchestratorQueue = Queue1::Manager<SendToBoardNotf>::create("(OrchestratorTask)orchestratorQueue");
    }

    void initialise()
    {
      notification.correlationId = 0;
    }

    bool timeToDowork()
    {
      return since_last_notification > sendInterval && thisTask->enabled;
    }

    void doWork()
    {
      if (orchestratorQueue == nullptr)
      {
        Serial.printf("ERROR: orchestratorQueue is NULL\n");
        return;
      }
      since_last_notification = 0;
      orchestratorQueue->send_r(&notification, printSendToQueue ? QueueBase::printSend : nullptr);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start()
  {
    thisTask = new TaskBase("OrchestratorTask", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }
}