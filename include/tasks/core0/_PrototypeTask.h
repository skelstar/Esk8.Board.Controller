#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

class PrototypeTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  // BatteryInfo Prototype;

  Queue1::Manager<BatteryInfo> *PrototypeBatteryQueue = nullptr;

public:
  PrototypeTask(unsigned long p_doWorkInterval) : TaskBase("PrototypeTask", 3000, p_doWorkInterval)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_0;
  }

  void initialiseQueues()
  {
    // PrototypeBatteryQueue = createQueueManager<BatteryInfo>("(PrototypeTask) PrototypeBatteryQueue");
  }

  void initialise()
  {
    // battery = new BatteryLib(34);
  }

  void initialTask()
  {
    // doWork();
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    // PrototypeBatteryQueue->send(&Prototype);
  }
};

PrototypeTask prototypeTask(5 * SECONDS);

namespace nsPrototypeTask
{
  void task1(void *parameters)
  {
    PrototypeTask.task(parameters);
  }
}
