#pragma once

#include <TaskBaseAlt.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <tasks/queues/types/BatteryInfo.h>

#include <BatteryLib.h>

class RemoteTaskA : public TaskBaseAlt
{
public:
  bool printWarnings = true;

private:
  elapsedMillis since_last_did_work = 0;
  BatteryLib *battery;
  BatteryInfo remote;
  Queue1::Manager<BatteryInfo> *remoteBatteryQueue = nullptr;

public:
  RemoteTaskA() : TaskBaseAlt("RemoteTaskAlt", 3000)
  {
    battery = new BatteryLib(34);
  }

  void initialiseQueues()
  {
    remoteBatteryQueue = createQueue<BatteryInfo>("(RemoteTaskA) remoteBatteryQueue");
  }

  void initialise()
  {
    battery->setup(nullptr);
  }

  void firstTask()
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

    remote.print("[RemoteTaskA]");
  }

  void start(uint8_t priority, ulong p_doWorkInterval, TaskFunction_t taskRef)
  {
    doWorkInterval = p_doWorkInterval;

    // xTaskCreatePinnedToCore(
    //     taskRef,
    //     "RemoteAlt",
    //     3000,
    //     NULL,
    //     priority,
    //     NULL,
    //     CORE_0);

    rtos->create(taskRef, CORE_0, priority, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (rtos != nullptr)
      rtos->deleteTask(print);
  }
};
