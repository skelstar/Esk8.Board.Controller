#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

#ifndef MAGNETIC_THROTTLE_H
// in case a mock is being used
// DEBUG("Using real MagThrottle library");
#include <MagThrottle.h>
#endif

namespace ThrottleTask
{
  struct Config
  {
    bool printWarnings = true;
    bool printThrottle = false;
  } settings;
  // prototypes
  TaskBase *thisTask;

  ThrottleState throttle;
  PrimaryButtonState primaryButton;

  namespace _p
  {
    Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
    Queue1::Manager<ThrottleState> *throttleQueue = nullptr;

    bool throttleEnabled_cb()
    {
      return primaryButton.pressed;
    };

    void initialiseQueues()
    {
      primaryButtonQueue = createQueue<PrimaryButtonState>("(throttle)primaryButtonQueue");
      throttleQueue = createQueue<ThrottleState>("(throttle)throttleQueue");
    }

    void initialise()
    {
      if (mux_I2C == nullptr)
        mux_I2C = xSemaphoreCreateMutex();

      MagneticThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
      MagneticThrottle::setThrottleEnabledCb(throttleEnabled_cb);
      MagneticThrottle::printThrottle = settings.printThrottle;
    }

    bool timeToDoWork()
    {
      return true;
    }

    void doWork()
    {
      if (primaryButtonQueue->hasValue())
      {
        primaryButton.event_id = primaryButtonQueue->payload.event_id;
        primaryButton.pressed = primaryButtonQueue->payload.pressed;
        // PrimaryButtonState::printRead(primaryButtonQueue->payload);
      }

      // check magthrottle
      MagneticThrottle::update(settings.printWarnings);
      throttleQueue->send(&throttle);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start(uint8_t priority, ulong doWorkInterval)
  {
    thisTask = new TaskBase("NintendoClassicTaskBase", 3000);
    thisTask->setInitialiseCallback(_p::initialise);
    thisTask->setInitialiseQueuesCallback(_p::initialiseQueues);
    thisTask->setTimeToDoWorkCallback(_p::timeToDoWork);
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
} // namespace