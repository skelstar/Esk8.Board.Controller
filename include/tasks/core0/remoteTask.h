#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <tasks/queues/types/BatteryInfo.h>

#include <BatteryLib.h>

class RemoteTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  BatteryLib *battery;
  BatteryInfo remote;

  Queue1::Manager<BatteryInfo> *remoteBatteryQueue = nullptr;

public:
  RemoteTask() : TaskBase("RemoteTask", 3000, PERIOD_50ms)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_0;
  }

  void initialiseQueues()
  {
    remoteBatteryQueue = createQueueManager<BatteryInfo>("(RemoteTask) remoteBatteryQueue");
  }

  void initialise()
  {
    battery = new BatteryLib(34);
    battery->setup(nullptr);
  }

  void initialTask()
  {
    doWork();
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    battery->update();

    remote.charging = battery->isCharging;
    remote.percent = battery->chargePercent;
    remote.volts = battery->getVolts();

    remoteBatteryQueue->send(&remote);
  }

  void cleanup()
  {
    delete (remoteBatteryQueue);
  }
};

RemoteTask remoteTask;

namespace nsRemoteTask
{
  void task1(void *parameters)
  {
    remoteTask.task(parameters);
  }
}
