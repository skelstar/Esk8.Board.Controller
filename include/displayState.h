#ifndef Fsm
#include <Fsm.h>
#endif

Fsm *displayState;

bool update_display = false;
Disp::Event lastDispEvent;

// prototypes

elapsedMillis sinceShowingToggleScreen;
elapsedMillis sinceStoredMovingTime;

//---------------------------------------------------------------
State dispState_searching([] {
  displayState->print("dispState_searching");
  screen_searching();
});
//---------------------------------------------------------------
State dispState_disconnected(
    [] {
      displayState->print("dispState_disconnected");
      screenWhenDisconnected();
    },
    NULL, NULL);
//---------------------------------------------------------------
State dispState_stoppedScreen(
    [] {
      displayState->print("dispState_stoppedScreen");
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
//---------------------------------------------------------------
State dispState_movingScreen(
    [] {
      displayState->print("dispState_movingScreen");
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
State dispState_needToAckResetsStopped(
    [] {
      displayState->print("dispState_needToAckResetsStopped");
#ifdef PRINT_RESET_DETECTION
      Serial.printf("ACK STOPPED at %lums\n", stats.timeMovingMS);
#endif
      screenNeedToAckResets();
    },
    NULL, NULL);
//---------------------------------------------------------------
State dispState_needToAckResetsMoving(
    [] {
      displayState->print("dispState_needToAckResetsMoving");
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
State dispState_boardVersionDoesntMatchScreen(
    [] {
      displayState->print("dispState_boardVersionDoesntMatchScreen");
      screenBoardNotCompatible(board.packet.version);
    },
    NULL,
    NULL);
//---------------------------------------------------------------

State dispState_togglePushToStart(
    [] {
      displayState->print("dispState_togglePushToStart");
      bool currVal = featureService.get<bool>(FeatureType::PUSH_TO_START);
      if (lastDispEvent == Disp::PRIMARY_DOUBLE_CLICK)
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
        lastDispEvent = Disp::PRIMARY_SINGLE_CLICK;
        displayState->trigger(Disp::PRIMARY_SINGLE_CLICK);
      }
    },
    NULL);
//---------------------------------------------------------------
void acknowledgeResets()
{
  stats.ackResets();
}
//---------------------------------------------------------------

void displayState_addTransitions()
{
  // Disp::CONNECTED
  displayState->add_transition(&dispState_searching, &dispState_stoppedScreen, Disp::CONNECTED, NULL);
  displayState->add_transition(&dispState_disconnected, &dispState_stoppedScreen, Disp::CONNECTED, NULL);

  // Disp::DISCONNECTED
  displayState->add_transition(&dispState_stoppedScreen, &dispState_disconnected, Disp::DISCONNECTED, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_disconnected, Disp::DISCONNECTED, NULL);

  // Disp::SW_RESET
  displayState->add_transition(&dispState_stoppedScreen, &dispState_needToAckResetsStopped, Disp::SW_RESET, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_needToAckResetsMoving, Disp::SW_RESET, NULL);

  displayState->add_transition(&dispState_needToAckResetsStopped, &dispState_stoppedScreen, Disp::PRIMARY_DOUBLE_CLICK, acknowledgeResets);
  displayState->add_transition(&dispState_needToAckResetsStopped, &dispState_needToAckResetsMoving, Disp::MOVING, NULL);
  displayState->add_transition(&dispState_needToAckResetsMoving, &dispState_needToAckResetsStopped, Disp::STOPPED, NULL);
  displayState->add_transition(&dispState_needToAckResetsMoving, &dispState_movingScreen, Disp::PRIMARY_DOUBLE_CLICK, acknowledgeResets);

  // Disp::MOVING
  displayState->add_transition(&dispState_stoppedScreen, &dispState_movingScreen, Disp::MOVING, NULL);
  // Disp::STOPPED
  displayState->add_transition(&dispState_movingScreen, &dispState_stoppedScreen, Disp::STOPPED, NULL);

  displayState->add_transition(&dispState_stoppedScreen, &dispState_togglePushToStart, Disp::PRIMARY_TRIPLE_CLICK, NULL);
  displayState->add_transition(&dispState_togglePushToStart, &dispState_togglePushToStart, Disp::PRIMARY_DOUBLE_CLICK, NULL);
  displayState->add_transition(&dispState_togglePushToStart, &dispState_stoppedScreen, Disp::PRIMARY_SINGLE_CLICK, NULL);

  // Disp::VERSION_DOESNT_MATCH
  displayState->add_transition(&dispState_stoppedScreen, &dispState_boardVersionDoesntMatchScreen, Disp::VERSION_DOESNT_MATCH, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_boardVersionDoesntMatchScreen, Disp::VERSION_DOESNT_MATCH, NULL);
}
//---------------------------------------------------------------