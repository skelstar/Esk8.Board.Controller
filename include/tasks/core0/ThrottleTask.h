#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <MagThumbwheel.h>

namespace nsThrottleTask
{
  PrimaryButtonState primaryButton;

  // bool throttleEnabled_cb()
  // {
  //   return true; // primaryButton.pressed;
  // };
}

class ThrottleTask : public TaskBase
{

public:
  bool printWarnings = true;
  bool printThrottle = false;

  MagneticThumbwheelClass thumbwheel;

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

    thumbwheel.setSweepAngle(SWEEP_ANGLE);
    thumbwheel.setAccelDirection(DIR_CLOCKWISE);
    // thumbwheel.setDeltaLimits(LIMIT_DELTA_MIN, LIMIT_DELTA_MAX);
    thumbwheel.setThrottleEnabledCb([] { return true; });
    thumbwheel.init(mux_I2C);

    thumbwheel.printThrottle = printThrottle;
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

      // changed and released?
      if (oldPressed != primaryButtonQueue->payload.pressed &&
          primaryButtonQueue->payload.pressed == false)
        thumbwheel.centre();
    }

    // check magthrottle
    uint8_t status = thumbwheel.update();
    throttle.val = thumbwheel.get();
    throttle.status = status;
    if (status == MagneticThumbwheelClass::MAG_NOT_DETECTED)
      Serial.printf("ERROR: magnet not detected!\n");

    // test to see if anything changed (maybe update^^ returns changed?)
    throttleQueue->send(&throttle);
  }

  void
  cleanup()
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
