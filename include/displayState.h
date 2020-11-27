#ifndef Fsm
#include <Fsm.h>
#endif

Fsm *displayState;

bool update_display = false;
DispStateEvent lastDispEvent;

// prototypes
void print_disp_state(const char *state_name, const char *event);
void print_disp_state(const char *state_name);
DispStateEvent read_from_display_event_queue(TickType_t ticks = 5);
void clearDisplayEventQueue();

elapsedMillis sinceShowingToggleScreen;
elapsedMillis sinceStoredMovingTime;

//---------------------------------------------------------------
State dispState_searching([] {
  print_disp_state("...dispState_searching");
  screen_searching();
});
//---------------------------------------------------------------
State dispState_disconnected(
    [] {
      print_disp_state("...dispState_disconnected");
      screenWhenDisconnected();
    },
    NULL, NULL);
//---------------------------------------------------------------
State dispState_stoppedScreen(
    [] {
      print_disp_state("...dispState_stoppedScreen", dispStateEventNames[(int)lastDispEvent]);
#ifdef PRINT_RESET_DETECTION
      Serial.printf("STOPPED at %lums\n", stats.timeMovingMS);
#endif
      screenWhenStopped(/*init*/ true);
    },
    [] {
      if (update_display || stats.update)
      {
        update_display = false;
        stats.update = false;
        screenWhenStopped(/*init*/ false);
        updateHudIcon(stats.hudConnected);
      }
    },
    NULL);
//---------------------------------------------------------------
State dispState_movingScreen(
    [] {
      print_disp_state("...dispState_movingScreen", dispStateEventNames[(int)lastDispEvent]);
#ifdef PRINT_RESET_DETECTION
      Serial.printf("MOVING at %lums\n", stats.timeMovingMS);
#endif
      sinceStoredMovingTime = 0;
      screenWhenMoving(/*init*/ true);
    },
    [] {
      if (update_display || stats.update)
      {
        update_display = false;
        stats.update = false;
        screenWhenMoving(/*init*/ false);
        updateHudIcon(stats.hudConnected);
      }

      stats.addMovingTime(sinceStoredMovingTime);
      sinceStoredMovingTime = 0;
    },
    [] {
    });

//---------------------------------------------------------------
State dispState_needToAckResetsStopped(
    [] {
      print_disp_state("...dispState_needToAckResetsStopped", dispStateEventNames[(int)lastDispEvent]);
#ifdef PRINT_RESET_DETECTION
      Serial.printf("ACK STOPPED at %lums\n", stats.timeMovingMS);
#endif
      screenNeedToAckResets();
    },
    NULL, NULL);
//---------------------------------------------------------------
State dispState_needToAckResetsMoving(
    [] {
      print_disp_state("...dispState_needToAckResetsMoving", dispStateEventNames[(int)lastDispEvent]);
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
      print_disp_state("...dispState_boardVersionDoesntMatchScreen", dispStateEventNames[(int)lastDispEvent]);
      screenBoardNotCompatible(board.packet.version);
    },
    NULL,
    NULL);
//---------------------------------------------------------------

State dispState_togglePushToStart(
    [] {
      print_disp_state("...dispState_togglePushToStart", dispStateEventNames[(int)lastDispEvent]);
      bool currVal = featureService.get<bool>(FeatureType::PUSH_TO_START);
      if (lastDispEvent == DISP_EV_PRIMARY_DOUBLE_CLICK)
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
        lastDispEvent = DISP_EV_PRIMARY_SINGLE_CLICK;
        displayState->trigger(DISP_EV_PRIMARY_SINGLE_CLICK);
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
  // DISP_EV_CONNECTED
  displayState->add_transition(&dispState_searching, &dispState_stoppedScreen, DISP_EV_CONNECTED, NULL);
  displayState->add_transition(&dispState_disconnected, &dispState_stoppedScreen, DISP_EV_CONNECTED, NULL);

  // DISP_EV_DISCONNECTED
  displayState->add_transition(&dispState_stoppedScreen, &dispState_disconnected, DISP_EV_DISCONNECTED, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_disconnected, DISP_EV_DISCONNECTED, NULL);

  // DISP_EV_SW_RESET
  displayState->add_transition(&dispState_stoppedScreen, &dispState_needToAckResetsStopped, DISP_EV_SW_RESET, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_needToAckResetsMoving, DISP_EV_SW_RESET, NULL);

  displayState->add_transition(&dispState_needToAckResetsStopped, &dispState_stoppedScreen, DISP_EV_PRIMARY_DOUBLE_CLICK, acknowledgeResets);
  displayState->add_transition(&dispState_needToAckResetsStopped, &dispState_needToAckResetsMoving, DISP_EV_MOVING, NULL);
  displayState->add_transition(&dispState_needToAckResetsMoving, &dispState_needToAckResetsStopped, DISP_EV_STOPPED, NULL);
  displayState->add_transition(&dispState_needToAckResetsMoving, &dispState_movingScreen, DISP_EV_PRIMARY_DOUBLE_CLICK, acknowledgeResets);

  // DISP_EV_MOVING
  displayState->add_transition(&dispState_stoppedScreen, &dispState_movingScreen, DISP_EV_MOVING, NULL);
  // DISP_EV_STOPPED
  displayState->add_transition(&dispState_movingScreen, &dispState_stoppedScreen, DISP_EV_STOPPED, NULL);

  displayState->add_transition(&dispState_stoppedScreen, &dispState_togglePushToStart, DISP_EV_PRIMARY_TRIPLE_CLICK, NULL);
  displayState->add_transition(&dispState_togglePushToStart, &dispState_togglePushToStart, DISP_EV_PRIMARY_DOUBLE_CLICK, NULL);
  displayState->add_transition(&dispState_togglePushToStart, &dispState_stoppedScreen, DISP_EV_PRIMARY_SINGLE_CLICK, NULL);

  // DISP_EV_UPDATE
  displayState->add_transition(&dispState_stoppedScreen, &dispState_stoppedScreen, DISP_EV_UPDATE, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_movingScreen, DISP_EV_UPDATE, NULL);

  // DISP_EV_VERSION_DOESNT_MATCH
  displayState->add_transition(&dispState_stoppedScreen, &dispState_boardVersionDoesntMatchScreen, DISP_EV_VERSION_DOESNT_MATCH, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_boardVersionDoesntMatchScreen, DISP_EV_VERSION_DOESNT_MATCH, NULL);
}
//---------------------------------------------------------------

void print_disp_state(const char *state_name, const char *event)
{
#ifdef PRINT_DISP_STATE
  Serial.printf("%s --> %s\n", state_name, sizeof(event) > 3 ? event : "EMPTY");
#endif
}
//---------------------------------------------------------------

void print_disp_state(const char *state_name)
{
#ifdef PRINT_DISP_STATE
  Serial.printf("%s\n", state_name);
#endif
}
//---------------------------------------------------------------
void clearDisplayEventQueue()
{
  while (read_from_display_event_queue() != DISP_EV_NO_EVENT)
  {
  }
}
//---------------------------------------------------------------