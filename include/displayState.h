#ifndef Fsm
#include <Fsm.h>
#endif
#ifndef FSMMANAGER_H
#include <FsmManager.h>
#endif

namespace Display
{
  bool update_display = false;
  DispState::Event lastDispEvent;

  elapsedMillis sinceShowingToggleScreen;
  elapsedMillis sinceStoredMovingTime;

  enum StateId
  {
    SEARCHING,
    DISCONNECTED,
    STOPPED_SCREEN,
    MOVING_SCREEN,
    NEED_TO_ACK_RESETS_STOPPED,
    NEED_TO_ACK_RESETS_MOVING,
    BOARD_VERSION_DOESNT_MATCH_SCREEN,
    TOGGLE_PUSH_TO_START,
    Length,
  };

  std::string stateNames[] = {
      "SEARCHING",
      "DISCONNECTED",
      "STOPPED_SCREEN",
      "MOVING_SCREEN",
      "NEED_TO_ACK_RESETS_STOPPED",
      "NEED_TO_ACK_RESETS_MOVING",
      "BOARD_VERSION_DOESNT_MATCH_SCREEN",
      "TOGGLE_PUSH_TO_START",
  };

  const char *getStateName(uint8_t id)
  {
    return id < Length && ARRAY_SIZE(stateNames) == Length
               ? stateNames[id].c_str()
               : OUT_OF_RANGE;
  }

  FsmManager<DispState::Event> dispFsm;

  //---------------------------------------------------------------
  State stateSearching([] {
    dispFsm.printState(SEARCHING);
    screen_searching();
  });
  //---------------------------------------------------------------
  State stateDisconnected(
      [] {
        dispFsm.printState(DISCONNECTED);
        screenWhenDisconnected();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateStoppedScreen(
      [] {
        dispFsm.printState(STOPPED_SCREEN);
        screenWhenStopped(/*init*/ true);
      },
      [] {
        if (update_display || stats.update)
        {
          update_display = false;
          stats.update = false;
          screenWhenStopped(/*init*/ false);
          updateHudIcon(hudClient.connected());
        }
      },
      NULL);
  //---------------------------------------------------------------
  State stateMovingScreen(
      [] {
        dispFsm.printState(MOVING_SCREEN);
        sinceStoredMovingTime = 0;
        screenWhenMoving(/*init*/ true);
      },
      [] {
        if (update_display || stats.update)
        {
          update_display = false;
          stats.update = false;
          screenWhenMoving(/*init*/ false);
          updateHudIcon(hudClient.connected());
        }

        stats.addMovingTime(sinceStoredMovingTime);
        sinceStoredMovingTime = 0;
      },
      NULL);
  //---------------------------------------------------------------
  State stateNeedToAckResetsStopped(
      [] {
        dispFsm.printState(NEED_TO_ACK_RESETS_STOPPED);
        screenNeedToAckResets();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateNeedToAckResetsMoving(
      [] {
        dispFsm.printState(NEED_TO_ACK_RESETS_MOVING);
        sinceStoredMovingTime = 0;
        screenNeedToAckResets();
      },
      [] {
        stats.addMovingTime(sinceStoredMovingTime);
        sinceStoredMovingTime = 0;
      },
      NULL);
  //---------------------------------------------------------------
  State stateBoardVersionDoesntMatchScreen(
      [] {
        dispFsm.printState(BOARD_VERSION_DOESNT_MATCH_SCREEN);
        screenBoardNotCompatible(board.packet.version);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------

  State stateTogglePushToStart(
      [] {
        dispFsm.printState(TOGGLE_PUSH_TO_START);
        bool currVal = featureService.get<bool>(FeatureType::PUSH_TO_START);
        if (lastDispEvent == DispState::PRIMARY_DOUBLE_CLICK)
        {
          currVal = !currVal;
          featureService.set(PUSH_TO_START, currVal);
        }
        screenPropValue("push to start", (currVal == true) ? "ON" : "OFF");
        sinceShowingToggleScreen = 0;
      },
      [] {
        if (sinceShowingToggleScreen > 3000)
        {
          sinceShowingToggleScreen = 0; // prevent excessive re-trigger
          lastDispEvent = DispState::PRIMARY_SINGLE_CLICK;
          dispFsm.trigger(DispState::PRIMARY_SINGLE_CLICK);
        }
      },
      NULL);
  //---------------------------------------------------------------
  void acknowledgeResets()
  {
    stats.ackResets();
  }
  //---------------------------------------------------------------

  Fsm fsm(&stateSearching);

  void addTransitions()
  {
    // DispState::CONNECTED
    fsm.add_transition(&stateSearching, &stateStoppedScreen, DispState::CONNECTED, NULL);
    fsm.add_transition(&stateDisconnected, &stateStoppedScreen, DispState::CONNECTED, NULL);

    // DispState::DISCONNECTED
    fsm.add_transition(&stateStoppedScreen, &stateDisconnected, DispState::DISCONNECTED, NULL);
    fsm.add_transition(&stateMovingScreen, &stateDisconnected, DispState::DISCONNECTED, NULL);

    // DispState::SW_RESET
    fsm.add_transition(&stateStoppedScreen, &stateNeedToAckResetsStopped, DispState::SW_RESET, NULL);
    fsm.add_transition(&stateMovingScreen, &stateNeedToAckResetsMoving, DispState::SW_RESET, NULL);

    fsm.add_transition(&stateNeedToAckResetsStopped, &stateStoppedScreen, DispState::PRIMARY_DOUBLE_CLICK, acknowledgeResets);
    fsm.add_transition(&stateNeedToAckResetsStopped, &stateNeedToAckResetsMoving, DispState::MOVING, NULL);
    fsm.add_transition(&stateNeedToAckResetsMoving, &stateNeedToAckResetsStopped, DispState::STOPPED, NULL);
    fsm.add_transition(&stateNeedToAckResetsMoving, &stateMovingScreen, DispState::PRIMARY_DOUBLE_CLICK, acknowledgeResets);

    // DispState::MOVING
    fsm.add_transition(&stateStoppedScreen, &stateMovingScreen, DispState::MOVING, NULL);
    // DispState::STOPPED
    fsm.add_transition(&stateMovingScreen, &stateStoppedScreen, DispState::STOPPED, NULL);

    fsm.add_transition(&stateStoppedScreen, &stateTogglePushToStart, DispState::PRIMARY_TRIPLE_CLICK, NULL);
    fsm.add_transition(&stateTogglePushToStart, &stateTogglePushToStart, DispState::PRIMARY_DOUBLE_CLICK, NULL);
    fsm.add_transition(&stateTogglePushToStart, &stateStoppedScreen, DispState::PRIMARY_SINGLE_CLICK, NULL);

    // DispState::UPDATE
    fsm.add_transition(&stateStoppedScreen, &stateStoppedScreen, DispState::UPDATE, NULL);
    fsm.add_transition(&stateMovingScreen, &stateMovingScreen, DispState::UPDATE, NULL);

    // DispState::VERSION_DOESNT_MATCH
    fsm.add_transition(&stateStoppedScreen, &stateBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
    fsm.add_transition(&stateMovingScreen, &stateBoardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
  }
} // namespace Display