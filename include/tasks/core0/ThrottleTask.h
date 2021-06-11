#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

#ifdef USE_I2C_ANALOG_TRIGGER
#include <AnalogI2CTrigger.h>
#endif
#ifdef USE_ANALOG_TRIGGER
#include <AnalogThumbwheel.h>
#endif

#define THROTTLE_TASK

namespace nsThrottleTask
{
  PrimaryButtonState primaryButton;
}

class ThrottleTask : public TaskBase
{

public:
  bool printWarnings = true;
  bool printThrottle = true;

#if REMOTE_USED == NINTENDO_REMOTE
  MagneticThumbwheelClass thumbwheel;
#endif
#ifdef USE_I2C_ANALOG_TRIGGER
  AnalogI2CTriggerClass thumbwheel;
#endif
#ifdef USE_ANALOG_TRIGGER
  AnalogThumbwheelClass thumbwheel;
#endif

public:
  ThrottleTask() : TaskBase("ThrottleTask", 3000)
  {
    _core = CORE_0;
  }

private:
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleQueue = nullptr;
  Queue1::Manager<Transaction> *transactionQueue = nullptr;

  Transaction m_transaction;

  ThrottleState throttle;

  void initialiseQueues()
  {
    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(throttle)primaryButtonQueue");
    throttleQueue = createQueueManager<ThrottleState>("(throttle)throttleQueue");
    transactionQueue = createQueueManager<Transaction>("(DisplayBase)TransactionQueue");
    transactionQueue->printMissedPacket = false;
    transactionQueue->read();
  }

  void _initialise()
  {
    // thumbwheel.setThrottleEnabledCb([]
    //                                 { return m_transaction.moving; });

#if REMOTE_USED == NINTENDO_REMOTE
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    thumbwheel.printThrottle = printThrottle;
    thumbwheel.init(mux_I2C, SWEEP_ANGLE, DEADZONE, ACCEL_DIRECTION);
#endif
#ifdef USE_I2C_ANALOG_TRIGGER
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    thumbwheel.printThrottle = printThrottle;
    thumbwheel.init(mux_I2C, ACCEL_DIRECTION);
#endif
#ifdef USE_ANALOG_TRIGGER
    thumbwheel.init();
    thumbwheel.printThrottle = printThrottle;
#endif
  }

  void doWork()
  {
    if (primaryButtonQueue->hasValue())
      handlePrimaryButton(primaryButtonQueue->payload);

    // transaction
    if (transactionQueue->hasValue())
      handleTransaction(transactionQueue->payload);

    // check throttle/trigger
    uint8_t og_throttle = thumbwheel.get();
    uint8_t status = thumbwheel.update(m_transaction.moving);
    throttle.val = thumbwheel.get();
    throttle.status = status;

    if (og_throttle != throttle.val || throttle.status != ThrottleStatus::STATUS_OK)
    {
      throttleQueue->send(&throttle);
      if (throttle.val == 255)
      {
        ThrottleState::print(throttle, "[ThrottleTask]-->");
      }
    }
  }

  void cleanup()
  {
    delete (primaryButtonQueue);
  }

  void handlePrimaryButton(PrimaryButtonState &payload)
  {
    using namespace nsThrottleTask;
    bool oldPressed = primaryButton.pressed;
    primaryButton.event_id = payload.event_id;
    primaryButton.pressed = payload.pressed;

    bool buttonReleased = oldPressed != payload.pressed && payload.pressed == false;
    if (buttonReleased)
      thumbwheel.centre();
  }

  void handleTransaction(Transaction &transaction)
  {
    m_transaction = transaction;
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
