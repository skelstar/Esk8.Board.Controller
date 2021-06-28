#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <tasks/queues/types/BatteryInfo.h>

#include <BatteryLib.h>

#define REMOTE_TASK

class RemoteTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  BatteryLib *battery;

  Queue1::Manager<BatteryInfo> *remoteBatteryQueue = nullptr;

public:
  RemoteTask() : TaskBase("RemoteTask", 3000)
  {
    _core = CORE_0;
  }

  void initialiseQueues()
  {
    remoteBatteryQueue = createQueueManager<BatteryInfo>("(RemoteTask) remoteBatteryQueue");
  }

  void _initialise()
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

    remoteBatteryQueue->payload.charging = battery->isCharging;
    remoteBatteryQueue->payload.percent = battery->chargePercent;
    // round to one decimal place
    remoteBatteryQueue->payload.volts = floorf(battery->getVolts() * 10) / 10;

    remoteBatteryQueue->sendPayload();
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
