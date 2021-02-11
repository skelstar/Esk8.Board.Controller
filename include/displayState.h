#ifndef Fsm
#include <Fsm.h>
#endif
#ifndef FSMMANAGER_H
#include <FsmManager.h>
#endif

namespace Display
{
  bool update_display = false;
  DispState::Trigger lastDispEvent;

  elapsedMillis sinceShowingToggleScreen;

  enum StateId
  {
    START_UP,
    DISCONNECTED,
    SOFTWARE_STATS,
    BOARD_BATTERY,
    STOPPED_SCREEN,
    MOVING_SCREEN,
    BOARD_VERSION_DOESNT_MATCH_SCREEN,
    SHOW_PUSH_TO_START,
    SHOW_SETTINGS,
    TOGGLE_PUSH_TO_START,
  };

  const char *stateID(uint16_t id)
  {
    switch (id)
    {
    case START_UP:
      return "START_UP";
    case DISCONNECTED:
      return "DISCONNECTED";
    case SOFTWARE_STATS:
      return "SOFTWARE_STATS";
    case BOARD_BATTERY:
      return "BOARD_BATTERY";
    case STOPPED_SCREEN:
      return "STOPPED_SCREEN";
    case MOVING_SCREEN:
      return "MOVING_SCREEN";
    case BOARD_VERSION_DOESNT_MATCH_SCREEN:
      return "BOARD_VERSION_DOESNT_MATCH_SCREEN";
    case SHOW_PUSH_TO_START:
      return "SHOW_PUSH_TO_START";
    case SHOW_SETTINGS:
      return "SHOW_SETTINGS";
    case TOGGLE_PUSH_TO_START:
      return "TOGGLE_PUSH_TO_START";
    }
    return outOfRange("Display::stateID()");
  }

  FsmManager<DispState::Trigger> dispFsm;

  //---------------------------------------------------------------
  State stateStartup(
      [] {
        dispFsm.printState(START_UP);
        screenSoftwareStats();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateDisconnected(
      [] {
        dispFsm.printState(DISCONNECTED);
        screenWhenDisconnected();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stSoftwareStats(
      [] {
        dispFsm.printState(SOFTWARE_STATS);
        screenSoftwareStats();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stBoardBattery(
      [] {
        dispFsm.printState(BOARD_BATTERY);
        if (Board::mutex1.take(__func__))
        {
          screenBoardBattery(board.packet.batteryVoltage);
          Board::mutex1.give(__func__);
        }
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateStopped(
      [] {
        if (dispFsm.lastEvent() != DispState::UPDATE)
          dispFsm.printState(STOPPED_SCREEN);

        const char *context = "disp: stateStopped (onEnter)";
        if (Stats::mutex.take(context, TICKS_10))
        {
          if (stats.controllerResets > 0)
            screenNeedToAckResets(Stats::CONTROLLER_RESETS);
          else if (stats.boardResets > 0)
            screenNeedToAckResets(Stats::BOARD_RESETS);
          else
            screenWhenStopped(/*init*/ true);
          Stats::mutex.give(context);
        }
      },
      [] {
        const char *context = "disp: stateStopped (onState)";
        if (Stats::mutex.take(context), TICKS_10)
        {
          if (stats.controllerResets == 0 && stats.boardResets == 0)
          {
            if (update_display || stats.update)
            {
              update_display = false;
              stats.update = false;
              screenWhenStopped(/*init*/ false);
            }
          }
          Stats::mutex.give(context);
        }
      },
      NULL);
  //---------------------------------------------------------------
  State stateMoving(
      [] {
        dispFsm.printState(MOVING_SCREEN);
        const char *context = "disp: stateMoving (onEnter)";
        if (Stats::mutex.take(context, TICKS_10))
        {
          if (stats.controllerResets > 0)
            screenNeedToAckResets(Stats::CONTROLLER_RESETS);
          else if (stats.boardResets > 0)
            screenNeedToAckResets(Stats::BOARD_RESETS);
          else
            screenWhenMoving(/*init*/ true);
          Stats::mutex.give(context);
        }
      },
      [] {
        const char *context = "disp: stateMoving (onState)";
        if (Stats::mutex.take(context, TICKS_10))
        {
          if (stats.controllerResets == 0)
          {
            if (update_display || stats.update)
            {
              update_display = false;
              stats.update = false;
              screenWhenMoving(/*init*/ false);
            }
          }
          Stats::mutex.give(context);
        }
      },
      NULL);
  //---------------------------------------------------------------
  State stateBoardVersionDoesntMatchScreen(
      [] {
        dispFsm.printState(BOARD_VERSION_DOESNT_MATCH_SCREEN);
        if (Board::mutex1.take(__func__))
        {
          screenBoardNotCompatible(board.packet.version);
          Board::mutex1.give(__func__);
        }
      },
      NULL,
      NULL);
  //---------------------------------------------------------------

#include "OptionsClass.h"

  //---------------------------------------------------------------
  State stShowSettings(
      [] {
        dispFsm.printState(SHOW_SETTINGS);
        sinceShowingToggleScreen = 0;
        switch (lastDispEvent)
        {
        case DispState::PRIMARY_TRIPLE_CLICK:
        {
          settingPtr = SettingOption::OPT_PUSH_TO_START;
          optionHandlers[settingPtr]->display();
          break;
        }
        case DispState::PRIMARY_SINGLE_CLICK:
        {
          nextSetting();
          optionHandlers[settingPtr]->display();
          break;
        }
        case DispState::PRIMARY_LONG_PRESS:
        {
          optionHandlers[settingPtr]->changeValue();
          optionHandlers[settingPtr]->display();
          break;
        }
        }
      },
      NULL, NULL);
  //---------------------------------------------------------------

  Fsm fsm(&stateStartup);

  void clearResetCounters()
  {
    if (Stats::mutex.take("disp: clearResetCounter"))
    {
      if (stats.controllerResets > 0)
        stats.clearControllerResets();
      else if (stats.boardResets > 0)
        stats.clearBoardResets();
      Stats::mutex.give("disp: clearResetCounter");
      Stats::queue->send(Stats::CLEAR_CONTROLLER_RESETS);
    }
  }

  void addTransitions()
  {
    fsm.add_timed_transition(&stateStartup, &stateDisconnected, 2000, NULL);

    // DispState::DISCONNECTED
    fsm.add_transition(&stateStopped, &stateDisconnected, DispState::DISCONNECTED, NULL);
    fsm.add_transition(&stateMoving, &stateDisconnected, DispState::DISCONNECTED, NULL);

    // DispState::PRIMARY_DOUBLE_CLICK
    fsm.add_transition(&stateStopped, &stateStopped, DispState::PRIMARY_DOUBLE_CLICK, clearResetCounters);
    fsm.add_transition(&stateMoving, &stateMoving, DispState::PRIMARY_DOUBLE_CLICK, clearResetCounters);

    // DispState::MOVING
    fsm.add_transition(&stateDisconnected, &stateMoving, DispState::MOVING, NULL);
    fsm.add_transition(&stateStopped, &stateMoving, DispState::MOVING, NULL);

    // DispState::STOPPED
    fsm.add_transition(&stateDisconnected, &stateStopped, DispState::STOPPED, NULL);
    fsm.add_transition(&stateMoving, &stateStopped, DispState::STOPPED, NULL);

    // settings
    /*
    - PRIMARY_TRIPLE_CLICK takes us to first settings screen
    - will do back to Stopped screen after 3 seconds if nothing done
    - PRIMARY_SINGLE_CLICK retriggers stShowSettings (navigate to next setting)
    - PRIMARY_LONG_PRESS retriggers stShowSettings (toggle value)
    */
    fsm.add_transition(&stateStopped, &stShowSettings, DispState::PRIMARY_TRIPLE_CLICK, NULL);
    fsm.add_timed_transition(&stShowSettings, &stateStopped, 3000, NULL);
    fsm.add_transition(&stShowSettings, &stShowSettings, DispState::PRIMARY_SINGLE_CLICK, NULL);
    fsm.add_transition(&stShowSettings, &stShowSettings, DispState::PRIMARY_LONG_PRESS, NULL);

    // DispState::UPDATE
    fsm.add_transition(&stateStopped, &stateStopped, DispState::UPDATE, NULL);
    fsm.add_transition(&stateMoving, &stateMoving, DispState::UPDATE, NULL);

    // DispState::VERSION_DOESNT_MATCH
    fsm.add_transition(&stateDisconnected, &stateBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
  }

  void queueReadCb(uint16_t ev)
  {
    if (PRINT_DISP_QUEUE_READ && ev != DispState::NO_EVENT)
      Serial.printf(PRINT_QUEUE_READ_FORMAT, "DISP", DispState::getTrigger(ev));
  }

} // namespace Display
