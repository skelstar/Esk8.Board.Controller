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

class DisplayTask : public TaskBase
{
public:
  bool p_printTrigger = false;
  bool p_printState = false;

private:
  DisplayEvent displayEvent;

  Queue1::Manager<BatteryInfo> *batteryQueue = nullptr;
  Queue1::Manager<PacketState> *packetStateQueue = nullptr;
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
  Queue1::Manager<NintendoButtonEvent> *nintendoClassicQueue = nullptr;
  Queue1::Manager<DisplayEvent> *displayEventQueue = nullptr;

  elapsedMillis since_checked_online = 0;

public:
  DisplayTask(unsigned long p_doWorkInterval) : TaskBase("DisplayTask", 5000, p_doWorkInterval)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_1;
  }
  //--------------------------------------------------

private:
  void initialiseQueues()
  {
    batteryQueue = createQueueManager<BatteryInfo>("(DisplayTask)BatteryInfo");
    packetStateQueue = createQueueManager<PacketState>("(DisplayBase)PacketStateQueue");
    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(DisplayBase)PrimaryButtonQueue");
    nintendoClassicQueue = createQueueManager<NintendoButtonEvent>("(DisplayBase)NintendoClassicQueue");
    displayEventQueue = createQueueManager<DisplayEvent>("(DisplayBase)DisplayEventQueue");
  }
  //--------------------------------
  void initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    setupLCD();

    Display::fsm_mgr.begin(&Display::_fsm);
    // Display::fsm_mgr.setPrintStateCallback(printState);
    // Display::fsm_mgr.setPrintTriggerCallback(printTrigger);

    Display::addTransitions();

    Display::_fsm.run_machine();
  }
  //--------------------------------

  bool timeToDoWork()
  {
    return since_checked_online > PERIOD_1s;
  }
  //--------------------------------
  void doWork()
  {
    // will check for online regardless of anything being new on the queue
    if (packetStateQueue->hasValue() && packetStateQueue->payload.event_id > 0)
      handlePacketState(packetStateQueue->payload);

    if (nintendoClassicQueue->hasValue())
      handleNintendoButtonEvent(nintendoClassicQueue->payload);

    if (batteryQueue->hasValue())
      handleBatteryQueue(batteryQueue->payload);

    Display::_fsm.run_machine();
  }

  void cleanup()
  {
    delete (batteryQueue);
    delete (packetStateQueue);
    delete (primaryButtonQueue);
    delete (nintendoClassicQueue);
    delete (displayEventQueue);
  }
  //==================================================

  void printState(uint16_t id)
  {
    if (p_printState)
      Serial.printf(PRINT_FSM_STATE_FORMAT, "DISP", millis(), Display::stateID(id));
  }

  void printTrigger(uint16_t ev)
  {
    if (p_printTrigger &&
        !(Display::_fsm.revisit() && ev == Display::TR_STOPPED) &&
        !(Display::_fsm.revisit() && ev == Display::TR_MOVING) &&
        !(Display::_fsm.getCurrentStateId() == Display::ST_OPTION_PUSH_TO_START && ev == Display::TR_STOPPED))
      Serial.printf(PRINT_FSM_TRIGGER_FORMAT, "DISP", millis(), Display::getTrigger(ev));
  }

#define NOT_IN_STATE(x) !Display::fsm_mgr.currentStateIs(x)
#define RESPONSE_WINDOW 200

  void handlePacketState(PacketState board)
  {
    if (board.connected(RESPONSE_WINDOW) == true)
    {
      // check version
      if (board.version != (float)VERSION_BOARD_COMPAT &&
          board.version > 0.0 &&
          NOT_IN_STATE(Display::ST_BOARD_VERSION_DOESNT_MATCH))
      {
        Display::_g_BoardVersion = board.version;
        Display::fsm_mgr.trigger(Display::TR_VERSION_DOESNT_MATCH);
      }
      // moving
      else if (NOT_IN_STATE(Display::ST_MOVING_SCREEN) && board.moving)
      {
        Display::fsm_mgr.trigger(Display::Trigger::TR_MOVING);
      }
      // stopped
      else if (NOT_IN_STATE(Display::ST_STOPPED_SCREEN) && !board.moving)
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

  void handleNintendoButtonEvent(NintendoButtonEvent payload)
  {
    if (payload.button == NintendoController::BUTTON_START)
      Display::fsm_mgr.trigger(Display::TR_MENU_BUTTON_CLICKED);

    // if (payload.button != NintendoController::BUTTON_NONE)
    //   Serial.printf("BUTTON: %s\n", NintendoController::getButtonName(payload.button));
  }

  void handleBatteryQueue(BatteryInfo battery)
  {
    bool changed = Display::_g_RemoteBattery.volts != battery.volts;
    Display::_g_RemoteBattery.charging = battery.charging;
    Display::_g_RemoteBattery.percent = battery.percent;
    Display::_g_RemoteBattery.volts = battery.volts;

    if (changed)
      Display::fsm_mgr.trigger(Display::TR_REMOTE_BATTERY_CHANGED);
  }
};

DisplayTask displayTask(PERIOD_50ms);

namespace Display
{
  void task1(void *parameters)
  {
    displayTask.task(parameters);
  }
}
