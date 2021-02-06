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
  elapsedMillis sinceStoredMovingTime;

  enum StateId
  {
    DISCONNECTED,
    SOFTWARE_STATS,
    BOARD_BATTERY,
    STOPPED_SCREEN,
    MOVING_SCREEN,
    // NEED_TO_ACK_RESETS_STOPPED,
    // NEED_TO_ACK_RESETS_MOVING,
    BOARD_VERSION_DOESNT_MATCH_SCREEN,
    SHOW_PUSH_TO_START,
    SHOW_SETTINGS,
    TOGGLE_PUSH_TO_START,
  };

  const char *stateID(uint16_t id)
  {
    switch (id)
    {
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
    // case NEED_TO_ACK_RESETS_STOPPED:
    //   return "NEED_TO_ACK_RESETS_STOPPED";
    // case NEED_TO_ACK_RESETS_MOVING:
    //   return "NEED_TO_ACK_RESETS_MOVING";
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
        screenBoardBattery(board.packet.batteryVoltage);
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateStopped(
      [] {
        dispFsm.printState(STOPPED_SCREEN);
        if (stats.soft_resets > 0)
        {
          screenNeedToAckResets();
        }
        else
        {
          if (dispFsm.lastEvent() == DispState::CONNECTED && board.packet.moving)
            dispFsm.trigger(DispState::MOVING);
          else
            screenWhenStopped(/*init*/ true);
        }
      },
      [] {
        if (stats.soft_resets == 0)
        {
          if (update_display || stats.update)
          {
            update_display = false;
            stats.update = false;
            screenWhenStopped(/*init*/ false);
          }
        }
      },
      NULL);
  //---------------------------------------------------------------
  State stateMoving(
      [] {
        dispFsm.printState(MOVING_SCREEN);
        // sinceStoredMovingTime = 0;
        screenWhenMoving(/*init*/ true);
      },
      [] {
        if (update_display || stats.update)
        {
          update_display = false;
          stats.update = false;
          screenWhenMoving(/*init*/ false);
        }

        stats.addMovingTime(sinceStoredMovingTime);
        sinceStoredMovingTime = 0;
      },
      NULL);
  //---------------------------------------------------------------
  // State stateNeedToAckResetsStopped(
  //     [] {
  //       dispFsm.printState(NEED_TO_ACK_RESETS_STOPPED);
  //       screenNeedToAckResets();
  //     },
  //     NULL, NULL);
  // //---------------------------------------------------------------
  // State stateNeedToAckResetsMoving(
  //     [] {
  //       dispFsm.printState(NEED_TO_ACK_RESETS_MOVING);
  //       sinceStoredMovingTime = 0;
  //       screenNeedToAckResets();
  //     },
  //     [] {
  //       stats.addMovingTime(sinceStoredMovingTime);
  //       sinceStoredMovingTime = 0;
  //     },
  //     NULL);
  //---------------------------------------------------------------
  State stateBoardVersionDoesntMatchScreen(
      [] {
        dispFsm.printState(BOARD_VERSION_DOESNT_MATCH_SCREEN);
        screenBoardNotCompatible(board.packet.version);
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

  Fsm fsm(&stateDisconnected);

  void ackUnintendedResets()
  {
    stats.ackResets();
    DEBUGVAL(stats.soft_resets);
  }

  void addTransitions()
  {
    // DispState::CONNECTED
    fsm.add_transition(&stateDisconnected, &stateStopped, DispState::CONNECTED, NULL);
    fsm.add_transition(&stateDisconnected, &stateStopped, DispState::UNINTENDED_RESET, NULL);

    // DispState::DISCONNECTED
    fsm.add_transition(&stateStopped, &stateDisconnected, DispState::DISCONNECTED, NULL);
    fsm.add_transition(&stateMoving, &stateDisconnected, DispState::DISCONNECTED, NULL);

    // DispState::UNINTENDED_RESET
    fsm.add_transition(&stateStopped, &stateStopped, DispState::UNINTENDED_RESET, NULL);
    fsm.add_transition(&stateMoving, &stateMoving, DispState::UNINTENDED_RESET, NULL);

    // DispState::PRIMARY_DOUBLE_CLICK
    fsm.add_transition(&stateStopped, &stateStopped, DispState::PRIMARY_DOUBLE_CLICK, ackUnintendedResets);
    fsm.add_transition(&stateMoving, &stateMoving, DispState::PRIMARY_DOUBLE_CLICK, ackUnintendedResets);

    // DispState::MOVING
    fsm.add_transition(&stateStopped, &stateMoving, DispState::MOVING, NULL);

    // DispState::STOPPED
    fsm.add_transition(&stateMoving, &stBoardBattery, DispState::STOPPED, NULL);

    fsm.add_timed_transition(&stBoardBattery, &stateStopped, 2000, NULL);

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
    fsm.add_transition(&stateStopped, &stateBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
    fsm.add_transition(&stateMoving, &stateBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
  }

  void queueReadCb(uint16_t ev)
  {
    if (PRINT_DISP_QUEUE_READ && ev != DispState::NO_EVENT)
      Serial.printf(PRINT_QUEUE_READ_FORMAT, "DISP", DispState::getTrigger(ev));
  }

} // namespace Display
