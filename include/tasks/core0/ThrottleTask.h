#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

namespace nsThrottleTask
{
  PrimaryButtonState primaryButton;
}

class ThrottleTask : public TaskBase
{

public:
  bool printWarnings = true;
  bool printThrottle = false;

#if REMOTE_USED == NINTENDO_REMOTE
  MagneticThumbwheelClass thumbwheel;
#elif REMOTE_USED == RED_REMOTE
  AnalogThumbwheelClass thumbwheel;
#endif

public:
  ThrottleTask() : TaskBase("ThrottleTask", 3000, PERIOD_50ms)
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

  void _initialise()
  {
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    // thumbwheel.setSweepAngle(SWEEP_ANGLE);
    // thumbwheel.setAccelDirection(DIR_CLOCKWISE);
    // thumbwheel.setDeltaLimits(LIMIT_DELTA_MIN, LIMIT_DELTA_MAX);
    thumbwheel.setThrottleEnabledCb([] { return true; });
    thumbwheel.printThrottle = printThrottle;
    thumbwheel.init(mux_I2C, SWEEP_ANGLE, DEADZONE, ACCEL_DIRECTION);
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
    uint8_t og_throttle = thumbwheel.get();
    uint8_t status = thumbwheel.update();
    throttle.val = thumbwheel.get();
    throttle.status = status;

    if (og_throttle != throttle.val || throttle.status != ThrottleStatus::STATUS_OK)
    {
      throttleQueue->send(&throttle);
      // ThrottleState::print(throttle, "[ThrottleTask]-->");
    }
  }

  void
  cleanup()
  {
    delete (primaryButtonQueue);
  }
};

ThrottleTask throttleTask;

namespace nsThrottleTask
{
  void task1(void *parameters)
  {
    throttleTask.task(parameters);
  }
}
