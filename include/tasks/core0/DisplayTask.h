#pragma once

#include <TaskBase.h>
// #include <tasks/queues/queues.h>
#include <tasks/queues/types/DisplayEvent.h>
#include <tasks/queues/QueueFactory.h>

#include <display/displayState.h>
#include <TFT_eSPI.h>
#include <display/tft.h>
#include <printFormatStrings.h>
#include <NintendoController.h>

#define DISPLAY_TASK

class DisplayTask : public TaskBase
{
public:
  bool p_printTrigger = false;
  bool p_printState = false;

private:
  DisplayEvent displayEvent;

  Queue1::Manager<BatteryInfo> *batteryQueue = nullptr;
  Queue1::Manager<Transaction> *transactionQueue = nullptr;
  Queue1::Manager<NintendoButtonEvent> *nintendoClassicQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleQueue = nullptr;
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

  Transaction transaction;

  bool _g_PrimaryButtonPressed = false,
       _g_PrimaryButtonHeld = false;

  elapsedMillis since_checked_online = 0,
                sinceTaskStarted,
                sincePrimaryButtonPressed;

public:
  DisplayTask() : TaskBase("DisplayTask", 10000)
  {
    _core = CORE_0;
  }
  //--------------------------------------------------

private:
  void initialiseQueues()
  {
    batteryQueue = createQueueManager<BatteryInfo>("(DisplayTask)BatteryInfo");
    batteryQueue->printMissedPacket = false;
    transactionQueue = createQueueManager<Transaction>("(DisplayBase)TransactionQueue");
    transactionQueue->printMissedPacket = false;
    nintendoClassicQueue = createQueueManager<NintendoButtonEvent>("(DisplayBase)NintendoClassicQueue");
    nintendoClassicQueue->printMissedPacket = false;
    throttleQueue = createQueueManager<ThrottleState>("(DisplayTask)ThrottleQueue");
    throttleQueue->printMissedPacket = false;
    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(DisplayTask)PrimaryButtonQueue");
    primaryButtonQueue->printMissedPacket = false;

    throttleQueue->read(); // clear queue
    primaryButtonQueue->read();
    transactionQueue->read();
  }
  //--------------------------------
  void _initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    setupLCD();

    Display::fsm_mgr.begin(&Display::_fsm);
    if (p_printState)
      Display::fsm_mgr.setPrintStateCallback(printState);
    if (p_printTrigger)
      Display::fsm_mgr.setPrintTriggerCallback(printTrigger);

    Display::addTransitions();

    Display::_fsm.run_machine();

    sinceTaskStarted = 0;
  }

  const unsigned long BATTERY_CHECK_INTERVAL = 5 * SECONDS;
  elapsedMillis sinceCheckedBattery = BATTERY_CHECK_INTERVAL - 100;

  //===================================================================
  void doWork()
  {
    // transaction
    if (transactionQueue->hasValue())
      handleTransaction(transactionQueue->payload);

    // nintendo classic
    if (nintendoClassicQueue->hasValue())
      handleNintendoButtonEvent(nintendoClassicQueue->payload);

    // battery
    if (sinceCheckedBattery > BATTERY_CHECK_INTERVAL)
    {
      sinceCheckedBattery = 0;
      if (batteryQueue->hasValue())
        handleBatteryQueue(batteryQueue->payload);
    }

    // throttle
    if (throttleQueue->hasValue())
      handleThrottleResponse(throttleQueue->payload);

    // primary button
    if (primaryButtonQueue->hasValue())
      handlePrimaryButton(primaryButtonQueue->payload);

    Display::_fsm.run_machine();
  }
  //===================================================================

  void cleanup()
  {
    delete (batteryQueue);
    delete (transactionQueue);
    delete (nintendoClassicQueue);
  }
  //==================================================

#define NOT_IN_STATE(x) !Display::fsm_mgr.currentStateIs(x)
#define IS_REVISIT() !Display::_fsm.revisit()
#define IN_STATE(x) Display::fsm_mgr.currentStateIs(x)
#define RESPONSE_WINDOW 500

  static void printState(uint16_t id)
  {
    Serial.printf(PRINT_FSM_STATE_FORMAT, "DISP", millis(), Display::stateID(id));
  }

  static void printTrigger(uint16_t ev)
  {
    if (!(IS_REVISIT() && ev == Display::TR_STOPPED) &&
        !(IS_REVISIT() && ev == Display::TR_MOVING) &&
        !(Display::_fsm.getCurrentStateId() == Display::ST_OPTION_PUSH_TO_START && ev == Display::TR_STOPPED))
      Serial.printf(PRINT_FSM_TRIGGER_FORMAT, "DISP", millis(), Display::getTrigger(ev));
  }

  void handleTransaction(Transaction &transaction)
  {
    // for some reason the battery votls were 0v in first packet
    // TODO try and work out why
    if (transaction.packet_id == 1)
      return;

    manageRunningState(transaction);

    if (sinceTaskStarted < 1000 && _g_PrimaryButtonPressed)
    {
      Display::fsm_mgr.trigger(Display::Trigger::TR_PRIMARY_BUTTON_HELD_ON_STARTUP);
    }

    if (transaction.connected(RESPONSE_WINDOW) == true)
    {
      Display::_g_BoardBattery = transaction.batteryVolts;

      if (!Display::_g_Connected)
      {
        // force redraw of battery
        sinceCheckedBattery = BATTERY_CHECK_INTERVAL + 100;
        Display::_g_Connected = true;
      }

      // check version
      if (transaction.version != (float)VERSION_BOARD_COMPAT &&
          transaction.version > 0.0 &&
          NOT_IN_STATE(Display::ST_BOARD_VERSION_DOESNT_MATCH))
      {
        Display::_g_BoardVersion = transaction.version;
        Display::fsm_mgr.trigger(Display::TR_VERSION_DOESNT_MATCH);
      }
      // moving
      else if (NOT_IN_STATE(Display::ST_MOVING_SCREEN) && transaction.moving)
      {
        Display::fsm_mgr.trigger(Display::Trigger::TR_MOVING);
      }
      // stopped
      else if (NOT_IN_STATE(Display::ST_STOPPED_SCREEN) && !transaction.moving)
      {
        Display::fsm_mgr.trigger(Display::Trigger::TR_STOPPED);
      }
    }
    // offline
    else
    {
      if (NOT_IN_STATE(Display::ST_DISCONNECTED))
      {
        Display::fsm_mgr.trigger(Display::TR_DISCONNECTED);
      }
    }
  }

  void handleNintendoButtonEvent(const NintendoButtonEvent &payload)
  {
    if (payload.button == NintendoController::BUTTON_START)
      Display::fsm_mgr.trigger(Display::TR_MENU_BUTTON_CLICKED);
    else if (payload.button == NintendoController::BUTTON_UP)
      Display::fsm_mgr.trigger(Display::TR_UP_BUTTON_CLICKED);
    else if (payload.button == NintendoController::BUTTON_RIGHT)
      Display::fsm_mgr.trigger(Display::TR_RIGHT_BUTTON_CLICKED);
    else if (payload.button == NintendoController::BUTTON_DOWN)
      Display::fsm_mgr.trigger(Display::TR_DOWN_BUTTON_CLICKED);
    else if (payload.button == NintendoController::BUTTON_LEFT)
      Display::fsm_mgr.trigger(Display::TR_LEFT_BUTTON_CLICKED);
    else if (payload.button == NintendoController::BUTTON_A)
      Display::fsm_mgr.trigger(Display::TR_A_BUTTON_CLICKED);
    else if (payload.button == NintendoController::BUTTON_B)
      Display::fsm_mgr.trigger(Display::TR_B_BUTTON_CLICKED);
    else if (payload.button == NintendoController::BUTTON_SELECT)
      Display::fsm_mgr.trigger(Display::TR_SEL_BUTTON_CLICKED);

    // if (payload.button != NintendoController::BUTTON_NONE)
    //   Serial.printf("BUTTON: %s\n", NintendoController::getButtonName(payload.button));
  }

  void handleBatteryQueue(const BatteryInfo &battery)
  {
    using namespace Display;
    bool changed = _g_RemoteBattery.volts != battery.volts; // volts are rounded to 1DP at this stage

    _g_RemoteBattery.charging = battery.charging;
    _g_RemoteBattery.percent = battery.percent;
    _g_RemoteBattery.volts = battery.volts;

    if (changed)
      fsm_mgr.trigger(TR_REMOTE_BATTERY_CHANGED);
  }

  void handleThrottleResponse(const ThrottleState &throttleState)
  {
    if (throttleState.status == ThrottleStatus::DIAL_DISCONNECTED && NOT_IN_STATE(Display::ST_MAGNET_NOT_DETECTED))
      Display::fsm_mgr.trigger(Display::TR_MAGNET_NOT_DETECTED);

    if (throttleState.status == ThrottleStatus::STATUS_OK && IN_STATE(Display::ST_MAGNET_NOT_DETECTED))
      Display::fsm_mgr.trigger(Display::TR_MAGNET_DETECTED);
  }

  void handlePrimaryButton(const PrimaryButtonState &primaryButton)
  {
    bool pressed = !_g_PrimaryButtonPressed && primaryButton.pressed,
         released = _g_PrimaryButtonPressed && !primaryButton.pressed,
         held = sincePrimaryButtonPressed > 1000;

    if (pressed)
    {
      Display::fsm_mgr.trigger(Display::TR_PRIMARY_BUTTON_PRESSED);
      sincePrimaryButtonPressed = 0;
    }
    else if (released)
    {
      _g_PrimaryButtonHeld = false;
    }
    else if (held && !_g_PrimaryButtonHeld)
    {
      Display::fsm_mgr.trigger(Display::TR_PRIMARY_BUTTON_HELD);
    }
    _g_PrimaryButtonPressed = primaryButton.pressed;
  }

  void manageRunningState(Transaction &transaction)
  {
    using namespace Display;

    bool moved = !_g_Moving && transaction.moving;
    bool stopped = _g_Moving && !transaction.moving;

    if (moved)
      setRunningState(RunningState::SLOW);
    else if (stopped)
      setRunningState(RunningState::FAST);
    _g_Moving = transaction.moving;
  }
};

DisplayTask displayTask;

namespace Display
{
  void task1(void *parameters)
  {
    displayTask.task(parameters);
  }
}
