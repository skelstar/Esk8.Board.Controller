#pragma once

#include <TaskBaseAlt.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

namespace nsThrottleTask
{
  PrimaryButtonState primaryButton;

  bool throttleEnabled_cb()
  {
    return primaryButton.pressed;
  };
}

class ThrottleTask : public TaskBaseAlt
{

public:
  enum PrintLevel
  {
    NONE = 0,
    THROTTLE,
    THROTTLE_DEBUG,
  };

  bool printWarnings = true;
  bool printThrottle = false;

public:
  ThrottleTask(unsigned long p_doWorkInterval) : TaskBaseAlt("ThrottleTask", 3000, p_doWorkInterval)
  {
  }

  void start(uint8_t priority, TaskFunction_t taskRef)
  {
    rtos->create(taskRef, CORE_0, priority, WITH_HEALTHCHECK);
  }

private:
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleQueue = nullptr;

  ThrottleState throttle;

  void initialiseQueues()
  {
    primaryButtonQueue = createQueue<PrimaryButtonState>("(throttle)primaryButtonQueue");
    throttleQueue = createQueue<ThrottleState>("(throttle)throttleQueue");
  }

  void initialise()
  {
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    Serial.printf("ThrottleTask init()\n");

    MagneticThrottle::setThrottleEnabledCb(nsThrottleTask::throttleEnabled_cb);
    MagneticThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
    MagneticThrottle::printThrottle = printThrottle;
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
          MagneticThrottle::centre();

      // PrimaryButtonState::printRead(primaryButtonQueue->payload);
    }

    // check magthrottle
    MagneticThrottle::update();
    // test to see if anything changed (maybe update^^ returns changed?)
    throttleQueue->send(&throttle);
  }

  void deleteTask(bool print = false)
  {
    exitTask = true;
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
