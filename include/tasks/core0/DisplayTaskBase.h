#pragma once

#include <TaskBaseAlt.h>
#include <tasks/queues/queues.h>
#include <tasks/queues/types/DisplayEvent.h>
#include <tasks/queues/QueueFactory.h>

#include <displayState.h>
#include <TFT_eSPI.h>
#include <tft.h>
#include <printFormatStrings.h>
#include <NintendoController.h>

class DisplayTask : public TaskBaseAlt
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
  DisplayTask(unsigned long p_doWorkInterval) : TaskBaseAlt("DisplayTask", 5000, p_doWorkInterval)
  {
  }
  //--------------------------------------------------
  void start(uint8_t priority, TaskFunction_t taskRef)
  {
    rtos->create(taskRef, CORE_0, priority, WITH_HEALTHCHECK);
  }

private:
  void initialiseQueues()
  {
    batteryQueue = createQueue<BatteryInfo>("(DisplayTask)BatteryInfo");
    packetStateQueue = createQueue<PacketState>("(DisplayBase)PacketStateQueue");
    primaryButtonQueue = createQueue<PrimaryButtonState>("(DisplayBase)PrimaryButtonQueue");
    nintendoClassicQueue = createQueue<NintendoButtonEvent>("(DisplayBase)NintendoClassicQueue");
    displayEventQueue = createQueue<DisplayEvent>("(DisplayBase)DisplayEventQueue");
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

  //--------------------------------------------------
  void deleteTask(bool print = false)
  {
    if (rtos != nullptr)
      rtos->deleteTask(print);
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

  void handlePacketState(PacketState board)
  {
    if (board.connected() == true)
    {
      // check version
      if (board.version != (float)VERSION_BOARD_COMPAT &&
          !Display::fsm_mgr.currentStateIs(Display::ST_BOARD_VERSION_DOESNT_MATCH_SCREEN))
      {
        Serial.printf("%.1f %.1f\n", board.version, (float)VERSION_BOARD_COMPAT);
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

    if (payload.button != NintendoController::BUTTON_NONE)
      Serial.printf("BUTTON: %s\n", NintendoController::getButtonName(payload.button));
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
