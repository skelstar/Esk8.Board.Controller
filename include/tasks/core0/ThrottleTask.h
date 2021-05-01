#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <MagThumbwheel.h>

namespace nsThrottleTask
{
  PrimaryButtonState primaryButton;

  bool throttleEnabled_cb()
  {
    return primaryButton.pressed;
  };
}

class ThrottleTask : public TaskBase
{

public:
  bool printWarnings = true;
  bool printThrottle = false;

public:
  ThrottleTask(unsigned long p_doWorkInterval) : TaskBase("ThrottleTask", 3000, p_doWorkInterval)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_4;
  }

private:
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleQueue = nullptr;

  ThrottleState throttle;

  MagneticThumbwheelClass magThrottle;

  void initialiseQueues()
  {
    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(throttle)primaryButtonQueue");
    throttleQueue = createQueueManager<ThrottleState>("(throttle)throttleQueue");
  }

  void initialise()
  {
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    Serial.printf("ThrottleTask init()\n");

    magThrottle.setSweepAngle(SWEEP_ANGLE);
    magThrottle.setAccelDirection(DIR_CLOCKWISE);
    // magThrottle.setDeltaLimits(LIMIT_DELTA_MIN, LIMIT_DELTA_MAX);
    magThrottle.setThrottleEnabledCb(nsThrottleTask::throttleEnabled_cb);
    magThrottle.init(mux_I2C);

    magThrottle.printThrottle = printThrottle;
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    if (primaryButtonQueue->hasValue())
    {
      bool oldPressed = nsThrottleTask::primaryButton.pressed;
      nsThrottleTask::primaryButton.event_id = primaryButtonQueue->payload.event_id;
      nsThrottleTask::primaryButton.pressed = primaryButtonQueue->payload.pressed;

      // changed?
      if (oldPressed != primaryButtonQueue->payload.pressed)
        // released?
        if (primaryButtonQueue->payload.pressed == false)
          magThrottle.centre();

      // PrimaryButtonState::printRead(primaryButtonQueue->payload);
    }

    // check magthrottle
    magThrottle.update();
    // test to see if anything changed (maybe update^^ returns changed?)
    throttleQueue->send(&throttle);
  }

  void cleanup()
  {
    delete (primaryButtonQueue);
  }
};

ThrottleTask throttleTask(PERIOD_200ms);

namespace nsThrottleTask
{
  void task1(void *parameters)
  {
    throttleTask.task(parameters);
  }
}
