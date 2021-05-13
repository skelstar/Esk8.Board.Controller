#pragma once

#include <TaskBase.h>
// #include <tasks/queues/queues.h>
#include <tasks/queues/types/DisplayEvent.h>
#include <tasks/queues/QueueFactory.h>

#include <displayState.h>
#include <TFT_eSPI.h>
#include <tft.h>
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
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
  Queue1::Manager<NintendoButtonEvent> *nintendoClassicQueue = nullptr;
  Queue1::Manager<DisplayEvent> *displayEventQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleQueue = nullptr;

  Transaction transaction;

  elapsedMillis since_checked_online = 0;

public:
  DisplayTask() : TaskBase("DisplayTask", 10000, PERIOD_50ms)
  {
    _core = CORE_0;
  }
  //--------------------------------------------------

private:
  void initialiseQueues()
  {
    batteryQueue = createQueueManager<BatteryInfo>("(DisplayTask)BatteryInfo");
    transactionQueue = createQueueManager<Transaction>("(DisplayBase)TransactionQueue");
    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(DisplayBase)PrimaryButtonQueue");
    nintendoClassicQueue = createQueueManager<NintendoButtonEvent>("(DisplayBase)NintendoClassicQueue");
    displayEventQueue = createQueueManager<DisplayEvent>("(DisplayBase)DisplayEventQueue");
    throttleQueue = createQueueManager<ThrottleState>("(DisplayTask)ThrottleQueue");

    throttleQueue->read(); // clear queue
    transactionQueue->read();
  }
  //--------------------------------
  void _initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    setupLCD();

    Display::fsm_mgr.begin(&Display::_fsm);
    Display::fsm_mgr.setPrintStateCallback(printState);
    Display::fsm_mgr.setPrintTriggerCallback(printTrigger);

    Display::addTransitions();

    Display::_fsm.run_machine();

    if (p_printState)
      Display::fsm_mgr.setPrintStateCallback(printState);
    if (p_printTrigger)
      Display::fsm_mgr.setPrintTriggerCallback(printTrigger);
  }

  //===================================================================
  void doWork()
  {
    // update transaction if anything new on the queue
    if (transactionQueue->hasValue())
    {
      transaction = transactionQueue->payload;
      Display::_g_BoardBattery = transaction.batteryVolts;

      handlePacketState(transaction);
    }

    if (nintendoClassicQueue->hasValue())
      handleNintendoButtonEvent(nintendoClassicQueue->payload);

    if (batteryQueue->hasValue())
      handleBatteryQueue(batteryQueue->payload);

    if (throttleQueue->hasValue())
      handleThrottleResponse(throttleQueue->payload);

    Display::_fsm.run_machine();
  }
  //===================================================================

  void cleanup()
  {
    delete (batteryQueue);
    delete (transactionQueue);
    delete (primaryButtonQueue);
    delete (nintendoClassicQueue);
    delete (displayEventQueue);
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

  void handlePacketState(Transaction &transaction)
  {
    if (transaction.connected(RESPONSE_WINDOW) == true)
    {
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
    bool changed = Display::_g_RemoteBattery.volts != battery.volts;
    Display::_g_RemoteBattery.charging = battery.charging;
    Display::_g_RemoteBattery.percent = battery.percent;
    Display::_g_RemoteBattery.volts = battery.volts;

    if (changed)
      Display::fsm_mgr.trigger(Display::TR_REMOTE_BATTERY_CHANGED);
  }

  void handleThrottleResponse(const ThrottleState &throttleState)
  {
    if (throttleState.status == ThrottleStatus::DIAL_DISCONNECTED && NOT_IN_STATE(Display::ST_MAGNET_NOT_DETECTED))
      Display::fsm_mgr.trigger(Display::TR_MAGNET_NOT_DETECTED);

    if (throttleState.status == ThrottleStatus::STATUS_OK && IN_STATE(Display::ST_MAGNET_NOT_DETECTED))
      Display::fsm_mgr.trigger(Display::TR_MAGNET_DETECTED);
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
