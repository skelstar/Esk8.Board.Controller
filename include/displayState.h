#ifndef Fsm
#include <Fsm.h>
#endif
#ifndef ENUMMANAGER_H
#include <EnumManager.h>
#endif

bool update_display = false;

// prototypes

elapsedMillis sinceShowingToggleScreen;
elapsedMillis sinceStoredMovingTime;

FsmManager dispFsm;

namespace Disp
{
  enum Event
  {
    NO_EVENT = 0,
    CONNECTED,
    DISCONNECTED,
    STOPPED,
    MOVING,
    SW_RESET,
    UPDATE,
    PRIMARY_SINGLE_CLICK,
    PRIMARY_DOUBLE_CLICK,
    PRIMARY_TRIPLE_CLICK,
    VERSION_DOESNT_MATCH,
    EventLength,
  };

  std::string eventNames[] = {
      "NO_EVENT",
      "CONNECTED",
      "DISCONNECTED",
      "STOPPED",
      "MOVING",
      "SW_RESET",
      "UPDATE",
      "PRIMARY_SINGLE_CLICK",
      "PRIMARY_DOUBLE_CLICK",
      "PRIMARY_TRIPLE_CLICK",
      "VERSION_DOESNT_MATCH",
  };

  const char *getName(int ev)
  {
    return ev >= 0 && ev < EventLength ? eventNames[ev].c_str() : "WARNING: OUT OF RANGE";
  }

  enum StateIDs
  {
    STATE_SEARCHING,
    STATE_DISCONNECTED,
    STATE_STOPPED,
    STATE_MOVING,
    STATE_NEEDTOACKRESETSSTOPPED,
    STATE_NEEDTOACKRESETSMOVING,
    STATE_BOARDVERSIONDOESNTMATCHSCREEN,
    STATE_TOGGLEPUSHTOSTART,
    STATE_Length,
  };

  std::string stateNames[] = {
      "STATE_SEARCHING",
      "STATE_DISCONNECTED",
      "STATE_STOPPED",
      "STATE_MOVING",
      "STATE_NEEDTOACKRESETSSTOPPED",
      "STATE_NEEDTOACKRESETSMOVING",
      "STATE_BOARDVERSIONDOESNTMATCHSCREEN",
      "STATE_TOGGLEPUSHTOSTART",
  };

  EnumManager<Event> eventsMgr(eventNames);
  EnumManager<StateIDs> stateIDsMgr(stateNames);

  State stateSearching(
      STATE_SEARCHING,
      [] {
        dispFsm.printState(STATE_SEARCHING);
        screen_searching();
      },
      NULL, NULL);

  State stateDisconnected(
      STATE_DISCONNECTED,
      [] {
        dispFsm.printState(STATE_DISCONNECTED);
        screenWhenDisconnected();
      },
      NULL, NULL);

  State stateStopped(
      STATE_STOPPED,
      [] {
        dispFsm.printState(STATE_STOPPED);
#ifdef PRINT_RESET_DETECTION
        Serial.printf("STOPPED at %lums\n", stats.timeMovingMS);
#endif
        screenWhenStopped(/*init*/ true);
      },
      [] {
        if (update_display)
        {
          update_display = false;
          screenWhenStopped(/*init*/ false);
        }
      },
      NULL);

  State stateMoving(
      STATE_MOVING,
      [] {
        dispFsm.printState(STATE_MOVING);
#ifdef PRINT_RESET_DETECTION
        Serial.printf("MOVING at %lums\n", stats.timeMovingMS);
#endif
        sinceStoredMovingTime = 0;
        screenWhenMoving(/*init*/ true);
      },
      [] {
        if (update_display)
        {
          update_display = false;
          screenWhenMoving(/*init*/ false);
        }

        stats.addMovingTime(sinceStoredMovingTime);
        sinceStoredMovingTime = 0;
      },
      [] {
      });

  //---------------------------------------------------------------
  State stateNeedToAckResetsStopped(
      STATE_NEEDTOACKRESETSSTOPPED,
      [] {
        dispFsm.printState(STATE_NEEDTOACKRESETSSTOPPED);

#ifdef PRINT_RESET_DETECTION
        Serial.printf("ACK STOPPED at %lums\n", stats.timeMovingMS);
#endif
        screenNeedToAckResets();
      },
      NULL, NULL);
  //---------------------------------------------------------------
  State stateNeedToAckResetsMoving(
      STATE_NEEDTOACKRESETSMOVING,
      [] {
        dispFsm.printState(STATE_NEEDTOACKRESETSMOVING);
#ifdef PRINT_RESET_DETECTION
        Serial.printf("ACK MOVING at %lums\n", stats.timeMovingMS);
#endif
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
      STATE_BOARDVERSIONDOESNTMATCHSCREEN,
      [] {
        dispFsm.printState(STATE_BOARDVERSIONDOESNTMATCHSCREEN);
        screenBoardNotCompatible(board.packet.version);
      },
      NULL,
      NULL);
  //---------------------------------------------------------------

  State stateTogglePushToStart(
      STATE_TOGGLEPUSHTOSTART,
      [] {
        dispFsm.printState(STATE_TOGGLEPUSHTOSTART);
        bool currVal = featureService.get<bool>(FeatureType::PUSH_TO_START);
        if (dispFsm.lastEvent() == Disp::PRIMARY_DOUBLE_CLICK)
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
          dispFsm.trigger(Disp::PRIMARY_SINGLE_CLICK);
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
    // Disp::CONNECTED
    fsm.add_transition(&stateSearching, &stateStopped, Disp::CONNECTED, NULL);
    fsm.add_transition(&stateDisconnected, &stateStopped, Disp::CONNECTED, NULL);

    // Disp::DISCONNECTED
    fsm.add_transition(&stateStopped, &stateDisconnected, Disp::DISCONNECTED, NULL);
    fsm.add_transition(&stateMoving, &stateDisconnected, Disp::DISCONNECTED, NULL);

    // Disp::SW_RESET
    fsm.add_transition(&stateStopped, &stateNeedToAckResetsStopped, Disp::SW_RESET, NULL);
    fsm.add_transition(&stateMoving, &stateNeedToAckResetsMoving, Disp::SW_RESET, NULL);

    fsm.add_transition(&stateNeedToAckResetsStopped, &stateStopped, Disp::PRIMARY_DOUBLE_CLICK, acknowledgeResets);
    fsm.add_transition(&stateNeedToAckResetsStopped, &stateNeedToAckResetsMoving, Disp::MOVING, NULL);
    fsm.add_transition(&stateNeedToAckResetsMoving, &stateNeedToAckResetsStopped, Disp::STOPPED, NULL);
    fsm.add_transition(&stateNeedToAckResetsMoving, &stateMoving, Disp::PRIMARY_DOUBLE_CLICK, acknowledgeResets);

    // Disp::MOVING
    fsm.add_transition(&stateStopped, &stateMoving, Disp::MOVING, NULL);
    // Disp::STOPPED
    fsm.add_transition(&stateMoving, &stateStopped, Disp::STOPPED, NULL);

    fsm.add_transition(&stateStopped, &stateTogglePushToStart, Disp::PRIMARY_TRIPLE_CLICK, NULL);
    fsm.add_transition(&stateTogglePushToStart, &stateTogglePushToStart, Disp::PRIMARY_DOUBLE_CLICK, NULL);
    fsm.add_transition(&stateTogglePushToStart, &stateStopped, Disp::PRIMARY_SINGLE_CLICK, NULL);

    // Disp::VERSION_DOESNT_MATCH
    fsm.add_transition(&stateStopped, &stateBoardVersionDoesntMatchScreen, Disp::VERSION_DOESNT_MATCH, NULL);
    fsm.add_transition(&stateMoving, &stateBoardVersionDoesntMatchScreen, Disp::VERSION_DOESNT_MATCH, NULL);
  }
} // namespace Disp
  //---------------------------------------------------------------