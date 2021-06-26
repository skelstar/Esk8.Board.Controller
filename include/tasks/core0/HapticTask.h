#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#ifdef USE_HAPTIC_I2C
#include <Haptic_Driver.h>
#endif
#include <Fsm.h>
#include <FsmManager.h>

#define HAPTIC_TASK

namespace nsHapticTask
{
#ifdef USE_HAPTIC_I2C
  Haptic_Driver haptic;
#endif

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
      details.period = PERIOD_200ms;
      details.pulses = 1;
      return details;
    case HapticCommand::HAP_TWO_SHORT_PULSES:
      details.command = command;
      details.strength = 80;
      details.period = PERIOD_200ms;
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
#ifdef USE_HAPTIC_I2C
    if (take(mux_I2C, ticks, __func__))
    {
      haptic.setVibrate(strength);
      give(mux_I2C);
    }
#endif
#ifdef USE_HAPTIC_DIGITAL
    digitalWrite(HAPTIC_DIGITAL_PIN, strength > 0 ? HIGH : LOW);
#endif
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
       printFsmTrigger = false,
       printMissedPacket = false;
  bool found = false;

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

#ifdef USE_HAPTIC_I2C
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();
#endif

    initialiseHaptic();

    nsHapticTask::fsm.begin(&_fsm);
    if (printFsmTrigger)
      fsm.setPrintTriggerCallback(nsHapticTask::printTrigger);
    addTransitions();

    nsHapticTask::startCommand(nsHapticTask::HAP_ONE_SHORT_PULSE);

    simplMsgQueue = createQueueManager<SimplMessageObj>("(HapticTask)simplMsgQueue");
    throttleQueue = createQueueManager<ThrottleState>("(HapticTask)throttleQueue");
    transactionQueue = createQueueManager<Transaction>("(HapticTask)transactionQueue");
    transactionQueue->printMissedPacket = printMissedPacket;
  }

  elapsedMillis sinceLastBuzz;

  void doWork()
  {
    // ignore if didn't find the haptic device (if using i2c)
    if (!found)
      return;

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
#ifdef USE_HAPTIC_I2C
    if (take(mux_I2C, TICKS_500ms, __func__))
    {
      hapticTask.found = haptic.begin();
      if (!hapticTask.found)
      {
        give(mux_I2C);
        Serial.printf("ERROR: Could not find Haptic unit\n");
        return;
      }
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
#endif
#ifdef USE_HAPTIC_DIGITAL
    pinMode(HAPTIC_DIGITAL_PIN, OUTPUT);
    hapticTask.found = true;
#endif
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
    // if (m_printDebug)
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