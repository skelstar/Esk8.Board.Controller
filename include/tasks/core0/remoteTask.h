#pragma once

#include <TaskBaseAlt.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <tasks/queues/types/BatteryInfo.h>

#include <BatteryLib.h>

class RemoteTask : public TaskBaseAlt
{
public:
  bool printWarnings = true;

private:
  BatteryLib *battery;
  BatteryInfo remote;

  Queue1::Manager<BatteryInfo> *remoteBatteryQueue = nullptr;

public:
  RemoteTask(unsigned long p_doWorkInterval) : TaskBaseAlt("RemoteTask", 3000, p_doWorkInterval)
  {
    battery = new BatteryLib(34);
  }

  void initialiseQueues()
  {
    remoteBatteryQueue = createQueue<BatteryInfo>("(RemoteTask) remoteBatteryQueue");
  }

  void initialise()
  {
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
    // remote.print("[RemoteTask]");
  }

  void start(uint8_t priority, TaskFunction_t taskRef)
  {
    rtos->create(taskRef, CORE_0, priority, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (rtos != nullptr)
      rtos->deleteTask(print);
  }
};

RemoteTask remoteTask(5 * SECONDS);

namespace Remote
{
  void task1(void *parameters)
  {
    remoteTask.task(parameters);
  }
}
