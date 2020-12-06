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
      if (lastDispEvent == Disp::DISP_EV_PRIMARY_DOUBLE_CLICK)
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
        lastDispEvent = Disp::DISP_EV_PRIMARY_SINGLE_CLICK;
        displayState->trigger(Disp::DISP_EV_PRIMARY_SINGLE_CLICK);
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
  // Disp::DISP_EV_CONNECTED
  displayState->add_transition(&dispState_searching, &dispState_stoppedScreen, Disp::DISP_EV_CONNECTED, NULL);
  displayState->add_transition(&dispState_disconnected, &dispState_stoppedScreen, Disp::DISP_EV_CONNECTED, NULL);

  // Disp::DISP_EV_DISCONNECTED
  displayState->add_transition(&dispState_stoppedScreen, &dispState_disconnected, Disp::DISP_EV_DISCONNECTED, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_disconnected, Disp::DISP_EV_DISCONNECTED, NULL);

  // Disp::DISP_EV_SW_RESET
  displayState->add_transition(&dispState_stoppedScreen, &dispState_needToAckResetsStopped, Disp::DISP_EV_SW_RESET, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_needToAckResetsMoving, Disp::DISP_EV_SW_RESET, NULL);

  displayState->add_transition(&dispState_needToAckResetsStopped, &dispState_stoppedScreen, Disp::DISP_EV_PRIMARY_DOUBLE_CLICK, acknowledgeResets);
  displayState->add_transition(&dispState_needToAckResetsStopped, &dispState_needToAckResetsMoving, Disp::DISP_EV_MOVING, NULL);
  displayState->add_transition(&dispState_needToAckResetsMoving, &dispState_needToAckResetsStopped, Disp::DISP_EV_STOPPED, NULL);
  displayState->add_transition(&dispState_needToAckResetsMoving, &dispState_movingScreen, Disp::DISP_EV_PRIMARY_DOUBLE_CLICK, acknowledgeResets);

  // Disp::DISP_EV_MOVING
  displayState->add_transition(&dispState_stoppedScreen, &dispState_movingScreen, Disp::DISP_EV_MOVING, NULL);
  // Disp::DISP_EV_STOPPED
  displayState->add_transition(&dispState_movingScreen, &dispState_stoppedScreen, Disp::DISP_EV_STOPPED, NULL);

  displayState->add_transition(&dispState_stoppedScreen, &dispState_togglePushToStart, Disp::DISP_EV_PRIMARY_TRIPLE_CLICK, NULL);
  displayState->add_transition(&dispState_togglePushToStart, &dispState_togglePushToStart, Disp::DISP_EV_PRIMARY_DOUBLE_CLICK, NULL);
  displayState->add_transition(&dispState_togglePushToStart, &dispState_stoppedScreen, Disp::DISP_EV_PRIMARY_SINGLE_CLICK, NULL);

  // Disp::DISP_EV_VERSION_DOESNT_MATCH
  displayState->add_transition(&dispState_stoppedScreen, &dispState_boardVersionDoesntMatchScreen, Disp::DISP_EV_VERSION_DOESNT_MATCH, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_boardVersionDoesntMatchScreen, Disp::DISP_EV_VERSION_DOESNT_MATCH, NULL);
}
//---------------------------------------------------------------