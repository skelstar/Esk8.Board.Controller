#include <TaskBase.h>
#include <QueueManager1.h>

namespace BaseTaskTest
{
  TaskBase *thisTask;

  namespace
  {
    Queue1::Manager<SendToBoardNotf> *notfQueue = nullptr;
    Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

    PrimaryButtonState state;

    void initialiseQueues()
    {
      Serial.printf("%s Initialised Queues\n", thisTask->_name);
      notfQueue = SendToBoardTimerTask::createQueueManager("(BaseTaskTest)NotfQueue");
      primaryButtonQueue = QwiicButtonTask::createQueueManager("(BaseTaskTest)primaryButtonQueue");
    }

    void initialise()
    {
      Serial.printf("%s Initialised\n", thisTask->_name);
    }

    bool timeToDowork()
    {
      if (notfQueue->hasValue())
      {
        DEBUGMVAL("timeToDOwOrk",
                  notfQueue->payload.correlationId,
                  notfQueue->payload.sent_time);
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
        DEBUG("ERROR: primaryButtonQueue is NULL");
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
    thisTask = new TaskBase("BaseTaskTest", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);
    thisTask->enabled = true;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
  }
}