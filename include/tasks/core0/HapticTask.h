#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <Haptic_Driver.h>

#define HAPTIC_TASK

class HapticTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
  Haptic_Driver haptic;

  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;
  Queue1::Manager<Transaction> *transactionQueue = nullptr;

  SimplMessageObj simplMessage;
  Transaction m_transaction;

public:
  HapticTask() : TaskBase("HapticTask", 3000, PERIOD_50ms)
  {
    _core = CORE_0;
  }

  void _initialise()
  {
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    if (take(mux_I2C, TICKS_500ms))
    {
      if (!haptic.begin())
        Serial.printf("Could not find Haptic unit\n");

      if (!haptic.defaultMotor())
        Serial.printf("Could not set default settings\n");

      haptic.enableFreqTrack(false);

      //s etting I2C Operation
      haptic.setOperationMode(DRO_MODE);
      vTaskDelay(TICKS_500ms);
      give(mux_I2C);
    }
    else
    {
      Serial.printf("[HapticTask] Unable to take mux_I2C\n");
    }

    simplMsgQueue = createQueueManager<SimplMessageObj>("(HapticTask)simplMsgQueue");

    transactionQueue = createQueueManager<Transaction>("(HapticTask)transactionQueue");
  }

  void doWork()
  {
    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    if (transactionQueue->hasValue())
      _handleTransaction(transactionQueue->payload);

    // TODO: set up fsm for haptic tasks maybe?
  }

  void cleanup()
  {
    delete (simplMsgQueue);
    delete (transactionQueue);
  }

  void _handleSimplMessage(SimplMessageObj simplMessage)
  {
  }

  void _handleTransaction(Transaction q_transaction)
  {
    if (m_transaction.moving != q_transaction.moving)
    {
      if (q_transaction.moving)
        _pulseHaptic(0.5, TICKS_100ms);
    }
    m_transaction = q_transaction;
  }

  int event = 0;

  void _pulseHaptic(float strength, TickType_t ticks)
  {
    if (strength > 1.0 || strength < 0.0)
    {
      Serial.printf("Haptic motor strength must be between 0.0 and 1.0 (0-100%)\n");
      return;
    }

    if (take(mux_I2C, TICKS_50ms))
    {
      // Max value is 127 with acceleration on (default).
      uint8_t s = 127 * strength;
      haptic.setVibrate(s);
      give(mux_I2C);
    }
    vTaskDelay(ticks);
    if (take(mux_I2C, TICKS_50ms))
    {
      haptic.setVibrate(0);
      give(mux_I2C);
    }
  }
};

HapticTask hapticTask;

namespace nsHapticTask
{
  void task1(void *parameters)
  {
    hapticTask.task(parameters);
  }
}
