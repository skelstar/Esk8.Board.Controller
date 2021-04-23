#pragma once

#include <TaskBase.h>
#include <tasks/queues/queues.h>
#include <tasks/queues/types/DisplayEvent.h>
#include <tasks/queues/QueueFactory.h>

#include <displayState.h>
#include <TFT_eSPI.h>
#include <tft.h>
#include <printFormatStrings.h>
#include <NintendoController.h>

namespace DisplayTaskBase
{
  // prototypes
  void printState(uint16_t id);
  void printTrigger(uint16_t ev);
  void handlePacketState(PacketState payload);
  void handleNintendoButtonEvent(NintendoButtonEvent payload);

  TaskBase *thisTask;

  DisplayEvent displayEvent;

  struct Config
  {
    bool printTrigger = false;
    bool printState = false;
  } settings;

  namespace _p
  {
    Queue1::Manager<PacketState> *packetStateQueue = nullptr;
    Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
    Queue1::Manager<NintendoButtonEvent> *nintendoClassicQueue = nullptr;
    Queue1::Manager<DisplayEvent> *displayEventQueue = nullptr;
    //--------------------------------
    void initialiseQueues()
    {
      packetStateQueue = createQueue<PacketState>("(DisplayBase)PacketStateQueue");
      primaryButtonQueue = createQueue<PrimaryButtonState>("(DisplayBase)PrimaryButtonQueue");
      nintendoClassicQueue = createQueue<NintendoButtonEvent>("(DisplayBase)NintendoClassicQueue");
      displayEventQueue = createQueue<DisplayEvent>("(DisplayBase)DisplayEventQueue");
    }
    //--------------------------------
    void initialise()
    {
      if (packetStateQueue == nullptr ||
          primaryButtonQueue == nullptr ||
          nintendoClassicQueue == nullptr ||
          displayEventQueue == nullptr)
        Serial.printf("[TASK:%s] one of the queues is NULL\n", thisTask->_name);

      if (mux_SPI == nullptr)
        mux_SPI = xSemaphoreCreateMutex();

      setupLCD();

      Display::fsm_mgr.begin(&Display::_fsm);
      Display::fsm_mgr.setPrintStateCallback(printState);
      Display::fsm_mgr.setPrintTriggerCallback(printTrigger);

      Display::addTransitions();

      Display::_fsm.run_machine();
    }

    elapsedMillis since_last_did_work = 0, since_checked_online = 0;

    bool timeToDowork()
    {
      return since_checked_online > PERIOD_1s && thisTask->enabled;
    }
    //--------------------------------
    void doWork()
    {
      packetStateQueue->hasValue();
      // will check for online regardless of anything being new on the queue
      if (packetStateQueue->payload.event_id > 0)
        handlePacketState(packetStateQueue->payload);

      if (nintendoClassicQueue->hasValue())
        handleNintendoButtonEvent(nintendoClassicQueue->payload);

      Display::_fsm.run_machine();
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }
  //--------------------------------
  void start(uint8_t priority, unsigned long doWorkInterval)
  {
    thisTask = new TaskBase("DisplayTaskBase", 3000);
    thisTask->setInitialiseQueuesCallback(_p::initialiseQueues);
    thisTask->setInitialiseCallback(_p::initialise);
    thisTask->setTimeToDoWorkCallback(_p::timeToDowork);
    thisTask->setDoWorkCallback(_p::doWork);

    thisTask->doWorkInterval = doWorkInterval > PERIOD_10ms ? doWorkInterval : PERIOD_10ms;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(_p::task, CORE_0, priority, WITH_HEALTHCHECK);
  }
  //--------------------------------------------------
  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }
  //==================================================

  void printState(uint16_t id)
  {
    if (settings.printState)
      Serial.printf(PRINT_FSM_STATE_FORMAT, "DISP", millis(), Display::stateID(id));
  }

  void printTrigger(uint16_t ev)
  {
    if (settings.printTrigger &&
        !(Display::_fsm.revisit() && ev == Display::TR_STOPPED) &&
        !(Display::_fsm.revisit() && ev == Display::TR_MOVING) &&
        !(Display::_fsm.getCurrentStateId() == Display::ST_OPTION_PUSH_TO_START && ev == Display::TR_STOPPED))
      Serial.printf(PRINT_FSM_TRIGGER_FORMAT, "DISP", millis(), Display::getTrigger(ev));
  }

  void handlePacketState(PacketState payload)
  {
    // check version
    if (payload.version != (float)VERSION_BOARD_COMPAT &&
        !Display::fsm_mgr.currentStateIs(Display::ST_BOARD_VERSION_DOESNT_MATCH_SCREEN))
    {
      Serial.printf("%.1f %.1f\n", payload.version, (float)VERSION_BOARD_COMPAT);
      Display::_g_BoardVersion = payload.version;
      Display::fsm_mgr.trigger(Display::TR_VERSION_DOESNT_MATCH);
    }
    // connected
    else if (payload.connected())
    {
      if (!Display::fsm_mgr.currentStateIs(Display::ST_MOVING_SCREEN) && payload.moving)
        Display::fsm_mgr.trigger(Display::Trigger::TR_MOVING);
      else if (!Display::fsm_mgr.currentStateIs(Display::ST_STOPPED_SCREEN) && !payload.moving)
        Display::fsm_mgr.trigger(Display::Trigger::TR_STOPPED);
    }
    // offline
    else
    {
      if (!Display::fsm_mgr.currentStateIs(Display::ST_DISCONNECTED))
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
}