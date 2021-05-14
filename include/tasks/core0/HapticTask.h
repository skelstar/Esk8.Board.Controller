#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <Haptic_Driver.h>

#define HAPTIC_TASK

namespace nsHapticTask
{
  Haptic_Driver haptic;

  enum HapticState
  {
    HAP_IDLE = 0,
    HAP_ON,
    HAP_OFF,
  };

  enum HapticCommand
  {
    HAP_NONE = 0,
    HAP_PULSE,
  };

  uint8_t m_strength = 0;
  unsigned long m_period = 0;
  HapticCommand m_command = HAP_NONE;
  HapticState m_state = HAP_IDLE;
  elapsedMillis sinceLastState = 0;

  // prototypes
  void startCommand(HapticCommand command, float strength, unsigned long period);
  void loop();
  void handlePulseCommand();
}

class HapticTask : public TaskBase
{
public:
  bool printWarnings = true;

private:
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
      if (!nsHapticTask::haptic.begin())
        Serial.printf("Could not find Haptic unit\n");

      if (!nsHapticTask::haptic.defaultMotor())
        Serial.printf("Could not set default settings\n");

      nsHapticTask::haptic.enableFreqTrack(false);

      //s etting I2C Operation
      nsHapticTask::haptic.setOperationMode(DRO_MODE);
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

    nsHapticTask::loop();
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
        nsHapticTask::startCommand(nsHapticTask::HAP_PULSE, 0.5, PERIOD_10ms);
    }
    m_transaction = q_transaction;
  }
};

HapticTask hapticTask;

namespace nsHapticTask
{
  void task1(void *parameters)
  {
    hapticTask.task(parameters);
  }

  void startCommand(HapticCommand command, float strength, unsigned long period)
  {
    m_command = command;
    if (strength > 1.0 || strength < 0.0)
    {
      Serial.printf("ERROR: Haptic motor strength must be between 0.0 and 1.0 (0-100%)\n");
      strength = 0.5;
    }

    m_strength = 127.0 * strength;
    m_period = period;
  }

  void loop()
  {
    if (m_command == HapticCommand::HAP_PULSE)
      handlePulseCommand();
  }

  void handlePulseCommand()
  {
    if (sinceLastState > m_period)
    {
      sinceLastState = 0;
      if (m_state == HAP_IDLE)
      {
        if (take(mux_I2C, TICKS_50ms))
        {
          haptic.setVibrate(m_strength);
          give(mux_I2C);
          m_state = HAP_ON;
        }
      }
      else if (m_state == HAP_ON)
      {
        if (take(mux_I2C, TICKS_500ms)) // really important
        {
          haptic.setVibrate(0);
          give(mux_I2C);
          m_command = HapticCommand::HAP_NONE;
          m_state = HAP_IDLE;
        }
      }
    }
  }
}
