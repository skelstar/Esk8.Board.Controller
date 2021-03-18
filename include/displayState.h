#ifndef Fsm
#include <Fsm.h>
#endif
#ifndef FSMMANAGER_H
#include <FsmManager.h>
#endif

namespace Display
{
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

  FsmManager<DispState::Trigger> fsm_mgr;

  //---------------------------------------------------------------
  State stateDisconnected(
      DISCONNECTED,
      [] {
        // TODO: make a param-less version of printState (i.e. if id set)
        fsm_mgr.printState(DISCONNECTED);
        screenWhenDisconnected();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stBoardBattery(
      BOARD_BATTERY,
      [] {
        fsm_mgr.printState(BOARD_BATTERY);
        screenBoardBattery(board.packet.batteryVoltage);
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stStopped(
      DispState::STOPPED,
      [] {
        // don't need UPDATE at this stage
        if (fsm_mgr.lastEvent() == DispState::UPDATE)
          return;

        fsm_mgr.printState(STOPPED_SCREEN);

        const char *context = "disp: stStopped (onEnter)";
        if (stats.controllerResets > 0)
          screenNeedToAckResets(Stats::CONTROLLER_RESETS);
        else if (stats.boardResets > 0)
          screenNeedToAckResets(Stats::BOARD_RESETS);
        else
          simpleStoppedScreen("STOPPED", TFT_CASET);
        // screenWhenStopped(/*init*/ true);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------
  State stMoving(
      DispState::MOVING,
      [] {
        // don't need UPDATE at this stage
        if (fsm_mgr.lastEvent() == DispState::UPDATE)
          return;

        fsm_mgr.printState(MOVING_SCREEN);

        const char *context = "disp: stMoving (onEnter)";
        if (stats.controllerResets > 0)
          screenNeedToAckResets(Stats::CONTROLLER_RESETS);
        else if (stats.boardResets > 0)
          screenNeedToAckResets(Stats::BOARD_RESETS);
        else
          simpleMovingScreen();
      },
      NULL,
      NULL);
  //---------------------------------------------------------------
  State stBoardVersionDoesntMatchScreen(
      BOARD_VERSION_DOESNT_MATCH_SCREEN,
      [] {
        fsm_mgr.printState(BOARD_VERSION_DOESNT_MATCH_SCREEN);
        screenBoardNotCompatible(board.packet.version);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------

#include "OptionsClass.h"

  //---------------------------------------------------------------
  State stShowSettings(
      SHOW_SETTINGS,
      [] {
        fsm_mgr.printState(SHOW_SETTINGS);
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

  Fsm _fsm(&stateDisconnected);

  void clearResetCounters()
  {
    if (stats.controllerResets > 0)
      stats.clearControllerResets();
    else if (stats.boardResets > 0)
      stats.clearBoardResets();
  }

  void addTransitions()
  {
    // DispState::DISCONNECTED
    _fsm.add_transition(&stStopped, &stateDisconnected, DispState::DISCONNECTED, NULL);
    _fsm.add_transition(&stMoving, &stateDisconnected, DispState::DISCONNECTED, NULL);

    // DispState::PRIMARY_DOUBLE_CLICK
    _fsm.add_transition(&stStopped, &stStopped, DispState::PRIMARY_DOUBLE_CLICK, clearResetCounters);
    _fsm.add_transition(&stMoving, &stMoving, DispState::PRIMARY_DOUBLE_CLICK, clearResetCounters);

    // DispState::MOVING
    _fsm.add_transition(&stateDisconnected, &stMoving, DispState::MOVING, NULL);
    _fsm.add_transition(&stStopped, &stMoving, DispState::MOVING, NULL);

    // DispState::STOPPED
    _fsm.add_transition(&stateDisconnected, &stStopped, DispState::STOPPED, NULL);
    _fsm.add_transition(&stMoving, &stStopped, DispState::STOPPED, NULL);

    //DispState::REMOTE_BATTERY_CHANGED
    _fsm.add_transition(&stStopped, &stStopped, DispState::REMOTE_BATTERY_CHANGED, NULL);

    // settings
    /*
    - PRIMARY_TRIPLE_CLICK takes us to first settings screen
    - will do back to Stopped screen after 3 seconds if nothing done
    - PRIMARY_SINGLE_CLICK retriggers stShowSettings (navigate to next setting)
    - PRIMARY_LONG_PRESS retriggers stShowSettings (toggle value)
    */
    _fsm.add_transition(&stStopped, &stShowSettings, DispState::PRIMARY_TRIPLE_CLICK, NULL);
    _fsm.add_timed_transition(&stShowSettings, &stStopped, 3000, NULL);
    _fsm.add_transition(&stShowSettings, &stShowSettings, DispState::PRIMARY_SINGLE_CLICK, NULL);
    _fsm.add_transition(&stShowSettings, &stShowSettings, DispState::PRIMARY_LONG_PRESS, NULL);

    // DispState::UPDATE
    _fsm.add_transition(&stStopped, &stStopped, DispState::UPDATE, NULL);
    _fsm.add_transition(&stMoving, &stMoving, DispState::UPDATE, NULL);

    // DispState::VERSION_DOESNT_MATCH
    _fsm.add_transition(&stateDisconnected, &stBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
  }
} // namespace Display
