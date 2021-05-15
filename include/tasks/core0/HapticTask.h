#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <Haptic_Driver.h>
#include <Fsm.h>
#include <FsmManager.h>

#define HAPTIC_TASK

namespace nsHapticTask
{
  Haptic_Driver haptic;

  bool m_printDebug = false;

  enum HapticState
  {
    HAP_IDLE = 0,
    HAP_ON,
    HAP_OFF,
  };

  enum HapticEvent
  {
    EV_IDLE = 0,
    EV_ON,
    EV_OFF,
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
  FsmManager<HapticEvent> fsm;

  // prototypes
  void initialiseHaptic();
  void startCommand(HapticCommand command, float strength, unsigned long period);

  void hapticSet(uint8_t strength, TickType_t ticks = TICKS_50ms)
  {
    if (take(mux_I2C, ticks, __func__))
    {
      haptic.setVibrate(strength);
      give(mux_I2C);
    }
  }

  void stateIdle_OnEnter()
  {
    if (m_printDebug)
      Serial.printf(PRINT_FSM_STATE_FORMAT, "HapticTask", millis(), "stateIdle");
    m_state = HapticState::HAP_IDLE;
    hapticSet(0);
  }

  void stateOn_OnEnter()
  {
    if (m_printDebug)
      Serial.printf(PRINT_FSM_STATE_FORMAT, "HapticTask", millis(), "stateOn");
    m_state = HapticState::HAP_ON;
    hapticSet(m_strength);
    sinceLastState = 0;
  }

  void stateOn_OnLoop()
  {
    if (sinceLastState > m_period)
    {
      if (m_command == HAP_PULSE)
        fsm.trigger(EV_IDLE);

      fsm.trigger(EV_IDLE);
    }
  }

  State stateIdle(stateIdle_OnEnter, NULL, NULL);
  State stateOn(stateOn_OnEnter, stateOn_OnLoop, NULL);

  Fsm _fsm(&stateIdle);

  void addTransitions()
  {
    _fsm.add_transition(&stateIdle, &stateOn, HapticEvent::EV_ON, NULL);
    _fsm.add_transition(&stateOn, &stateIdle, HapticEvent::EV_IDLE, NULL);
  }
}

class HapticTask : public TaskBase
{
public:
  bool printWarnings = true, printDebug = false;

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
    using namespace nsHapticTask;

    nsHapticTask::m_printDebug = printDebug;

    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    initialiseHaptic();

    nsHapticTask::fsm.begin(&_fsm);
    addTransitions();

    simplMsgQueue = createQueueManager<SimplMessageObj>("(HapticTask)simplMsgQueue");

    transactionQueue = createQueueManager<Transaction>("(HapticTask)transactionQueue");
  }

  void doWork()
  {
    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    if (transactionQueue->hasValue())
      _handleTransaction(transactionQueue->payload);

    nsHapticTask::_fsm.run_machine();
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

  void initialiseHaptic()
  {
    if (take(mux_I2C, TICKS_500ms, __func__))
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

    fsm.trigger(EV_ON);
    DEBUG("sent EV_ON");
  }
}
