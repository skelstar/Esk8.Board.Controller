#include <TaskBase.h>
#include <QueueManager1.h>

namespace BaseTaskTest
{
  TaskBase *thisTask;

  namespace
  {
    Queue1::Manager<SendToBoardNotf> *notfQueue = nullptr;

    void initialiseQueues()
    {
      Serial.printf("%s Initialised Queues\n", thisTask->_name);
      notfQueue = SendToBoardTimerTask::createQueueManager("BaseTaskTest)NotfQueue");
    }

    void initialise()
    {
      Serial.printf("%s Initialised\n", thisTask->_name);
    }

    bool timeToDowork()
    {
      return notfQueue != nullptr && notfQueue->hasValue();
    }

    void doWork()
    {
      Serial.printf("%s Doing work\n", thisTask->_name);
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

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
  }
}