#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <tasks/queues/types/BatteryInfo.h>

#include <BatteryLib.h>

namespace RemoteTask
{
  bool printWarnings = true;

  TaskBase *thisTask;

  BatteryLib battery(BATTERY_MEASURE_PIN);

  BatteryInfo remote;

  elapsedMillis since_measure_battery;

  namespace _p
  {
    // prototypes
    void doWork();

    Queue1::Manager<BatteryInfo> *remoteBatteryQueue = nullptr;

    void initialiseQueues()
    {
      remoteBatteryQueue = createQueue<BatteryInfo>("(RemoteTask) remoteBatteryQueue");
    }

    void initialise()
    {
      battery.setup(nullptr);
    }

    void firstTask()
    {
      doWork();
    }

    elapsedMillis since_last_did_work = 0;

    bool timeToDowork()
    {
      return true;
    }

    void doWork()
    {
      battery.update();

      remote.charging = battery.isCharging;
      remote.percent = battery.chargePercent;
      remote.volts = battery.getVolts();

      remoteBatteryQueue->send(&remote);

      remote.print("[RemoteTask]");
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start(uint8_t priority, ulong doWorkInterval)
  {
    thisTask = new TaskBase("RemoteTask", 3000);
    thisTask->setInitialiseCallback(_p::initialise);
    thisTask->setInitialiseQueuesCallback(_p::initialiseQueues);
    thisTask->setFirstTaskCallback(_p::firstTask);
    thisTask->setTimeToDoWorkCallback(_p::timeToDowork);
    thisTask->setDoWorkCallback(_p::doWork);

    thisTask->doWorkInterval = doWorkInterval;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(_p::task, CORE_0, priority, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }

  //--------------------------------------------------------
} // namespace Remote
