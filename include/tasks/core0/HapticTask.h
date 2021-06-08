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

  const char *getHapticEvent(int ev)
  {
    switch (ev)
    {
    case EV_IDLE:
      return "EV_IDLE";
    case EV_ON:
      return "EV_ON";
    case EV_OFF:
      return "EV_OFF";
    }
    return "OUT OF RANGE (getHapticEvent)";
  }

  enum HapticCommand
  {
    HAP_NONE = 0,
    HAP_ONE_SHORT_PULSE,
    HAP_TWO_SHORT_PULSES,
  };

  struct HapticCommandStruct
  {
    HapticCommand command = HapticCommand::HAP_NONE;
    uint8_t strength = 0;
    unsigned long period = 0;
    uint8_t pulses = 1;
  };

  static HapticCommandStruct CommandDetailsFactory(HapticCommand command)
  {
    HapticCommandStruct details;
    switch (command)
    {
    case HapticCommand::HAP_ONE_SHORT_PULSE:
      details.command = command;
      details.strength = 50;
      details.period = PERIOD_100ms;
      details.pulses = 1;
      return details;
    case HapticCommand::HAP_TWO_SHORT_PULSES:
      details.command = command;
      details.strength = 80;
      details.period = PERIOD_100ms;
      details.pulses = 2;
      return details;
    }
    return details;
  }

  HapticCommandStruct command;

  HapticState m_state = HAP_IDLE;
  elapsedMillis sinceLastState = 0;
  FsmManager<HapticEvent> fsm;

  // prototypes
  void initialiseHaptic();
  void startCommand(HapticCommand command);

  void hapticSet(uint8_t strength, TickType_t ticks = TICKS_50ms)
  {
    if (take(mux_I2C, ticks, __func__))
    {
      haptic.setVibrate(strength);
      give(mux_I2C);
    }
  }

  //---------------------------

  int numPulses = 0;

  void stateIdle_OnEnter();
  State stateIdle(stateIdle_OnEnter, NULL, NULL);

  void stateOn_OnEnter();
  void stateOn_Loop();
  State stateOn(stateOn_OnEnter, stateOn_Loop, NULL);

  void stateOff_OnEnter();
  void stateOff_Loop();
  State stateOff(stateOff_OnEnter, stateOff_Loop, NULL);

  Fsm _fsm(&stateIdle);

  void addTransitions()
  {
    // EV_ON
    _fsm.add_transition(&stateIdle, &stateOn, HapticEvent::EV_ON, NULL);
    _fsm.add_transition(&stateOff, &stateOn, HapticEvent::EV_ON, NULL);

    // EV_OFF
    _fsm.add_transition(&stateOn, &stateOff, HapticEvent::EV_OFF, NULL);

    // EV_IDLE
    _fsm.add_transition(&stateOn, &stateIdle, HapticEvent::EV_IDLE, NULL);
    _fsm.add_transition(&stateOff, &stateIdle, HapticEvent::EV_IDLE, NULL);
  }

  void printTrigger(uint16_t ev)
  {
    Serial.printf(PRINT_FSM_TRIGGER_FORMAT, "HapticTask", millis(), getHapticEvent(ev));
  }
}

class HapticTask : public TaskBase
{
public:
  bool printWarnings = true,
       printDebug = false,
       printFsmTrigger = false;

private:
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleQueue = nullptr;
  Queue1::Manager<Transaction> *transactionQueue = nullptr;

  SimplMessageObj simplMessage;
  ThrottleState _throttleState;
  Transaction m_transaction;

public:
  HapticTask() : TaskBase("HapticTask", 3000)
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
    if (printFsmTrigger)
      fsm.setPrintTriggerCallback(nsHapticTask::printTrigger);
    addTransitions();

    nsHapticTask::startCommand(nsHapticTask::HAP_ONE_SHORT_PULSE);

    simplMsgQueue = createQueueManager<SimplMessageObj>("(HapticTask)simplMsgQueue");
    throttleQueue = createQueueManager<ThrottleState>("(HapticTask)throttleQueue");
    transactionQueue = createQueueManager<Transaction>("(HapticTask)transactionQueue");
  }

  void doWork()
  {
    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    if (transactionQueue->hasValue())
      _handleTransaction(transactionQueue->payload);

    if (throttleQueue->hasValue())
      _handleThrottleState(throttleQueue->payload);

    nsHapticTask::_fsm.run_machine();
  }

  void cleanup()
  {
    delete (simplMsgQueue);
    delete (transactionQueue);
  }

  void _handleSimplMessage(SimplMessageObj &simplMessage)
  {
  }

  void _handleTransaction(Transaction &q_transaction)
  {
    if (q_transaction.reason == ReasonType::FIRST_PACKET)
    {
      nsHapticTask::startCommand(nsHapticTask::HAP_ONE_SHORT_PULSE);
    }
    m_transaction = q_transaction;
  }

  void _handleThrottleState(ThrottleState &state)
  {
    // if (_throttleState.val != state.val && state.val == 255)
    // {
    //   nsHapticTask::startCommand(nsHapticTask::HAP_ONE_SHORT_PULSE);
    // }
    _throttleState = state;
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
        Serial.printf("ERROR: Could not find Haptic unit\n");

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

  void startCommand(HapticCommand p_command)
  {
    nsHapticTask::command = nsHapticTask::CommandDetailsFactory(p_command);
    fsm.trigger(EV_ON);
  }

  //-----------------------------------------------------------------------------------------------------
  // stateIdle
  //-----------------------------------------------------------------------------------------------------
  void stateIdle_OnEnter()
  {
    if (m_printDebug)
      Serial.printf(PRINT_FSM_STATE_FORMAT, "HapticTask", millis(), "stateIdle");
    m_state = HapticState::HAP_IDLE;
    hapticSet(0);
    numPulses = 0;
  }
  //-----------------------------------------------------------------------------------------------------
  // stateOn
  //-----------------------------------------------------------------------------------------------------
  void stateOn_OnEnter()
  {
    if (m_printDebug)
      Serial.printf(PRINT_FSM_STATE_FORMAT, "HapticTask", millis(), "stateOn");
    m_state = HapticState::HAP_ON;
    hapticSet(command.strength);
    sinceLastState = 0;
  }

  void stateOn_Loop()
  {
    if (sinceLastState > command.period)
    {
      numPulses++;
      if (command.command == HAP_ONE_SHORT_PULSE)
        fsm.trigger(EV_IDLE);
      else if (command.command == HAP_TWO_SHORT_PULSES)
      {
        bool finished = numPulses >= command.pulses;
        fsm.trigger(!finished ? EV_OFF : EV_IDLE);
        fsm.runMachine();
      }
    }
  }
  //-----------------------------------------------------------------------------------------------------
  // stateOff
  //-----------------------------------------------------------------------------------------------------
  void stateOff_OnEnter()
  {
    if (m_printDebug)
      Serial.printf(PRINT_FSM_STATE_FORMAT, "HapticTask", millis(), "stateOff");
    m_state = HapticState::HAP_OFF;
    hapticSet(0);
    sinceLastState = 0;
  }

  void stateOff_Loop()
  {
    if (command.command == HAP_ONE_SHORT_PULSE)
      fsm.trigger(EV_IDLE);
    else if (sinceLastState > command.period)
    {
      sinceLastState = 0;
      if (command.command == HAP_TWO_SHORT_PULSES)
      {
        bool finished = numPulses >= command.pulses;
        fsm.trigger(!finished ? EV_ON : EV_IDLE);
        fsm.runMachine();
      }
    }
  }
}