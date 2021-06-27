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

class ThrottleTask : public TaskBase
{
public:
  bool printWarnings = true,
       printThrottle = false,
       printQueues = false;

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
    thumbwheel.centre();
    thumbwheel.setRawThrottleCallback(_printRawThrottle);
#endif
  }

  elapsedMillis sinceUpdatedThrottle = 0;

  void doWork()
  {
    if (primaryButtonQueue->hasValue())
      handlePrimaryButton(primaryButtonQueue->payload);

    // transaction
    if (transactionQueue->hasValue())
      handleTransaction(transactionQueue->payload);

    // check throttle/trigger
    if (sinceUpdatedThrottle > SEND_TO_BOARD_INTERVAL)
      _updateThrottle();
  }

  void cleanup()
  {
    delete (primaryButtonQueue);
  }

  void _updateThrottle()
  {
    sinceUpdatedThrottle = 0;
    bool throttleEnabled = true;
    bool accelEnabled = FEATURE_USE_DEADMAN == 1
                            ? primaryButtonQueue->payload.pressed
                            : transactionQueue->payload.moving || FEATURE_PUSH_TO_START == 0;

    uint8_t status = thumbwheel.update(throttleEnabled, accelEnabled);
    throttleQueue->payload.val = status == ThrottleStatus::STATUS_OK ? thumbwheel.get() : 127;
    throttleQueue->payload.status = status;

    throttleQueue->sendPayload(printQueues);
  }

  static void _printRawThrottle(uint8_t t, uint16_t raw, uint16_t centre)
  {
    Serial.printf("throttle:%d  |  raw:%d  |  centre:%d\n", t, raw, centre);
  };

  void handlePrimaryButton(PrimaryButtonState &payload)
  {
#if FEATURE_USE_DEADMAN == 0
    bool buttonReleased = oldPressed != payload.pressed && payload.pressed == false;
    if (buttonReleased)
      thumbwheel.centre();
#endif
  }

  void handleTransaction(Transaction &transaction)
  {
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
