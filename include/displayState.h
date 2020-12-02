#ifndef Fsm
#include <Fsm.h>
#endif

Fsm *displayState;

bool update_display = false;
DispState::Event lastDispEvent;

// prototypes
void print_disp_state(const char *state_name, const char *event);
void print_disp_state(const char *state_name);
DispState::Event read_from_display_event_queue(TickType_t ticks = 5);
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
      print_disp_state("...dispState_stoppedScreen", DispState::names[(int)lastDispEvent]);
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
State dispState_movingScreen(
    [] {
      print_disp_state("...dispState_movingScreen", DispState::names[(int)lastDispEvent]);
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
    [] {
    });

//---------------------------------------------------------------
State dispState_needToAckResetsStopped(
    [] {
      print_disp_state("...dispState_needToAckResetsStopped", DispState::names[(int)lastDispEvent]);
      screenNeedToAckResets();
    },
    NULL, NULL);
//---------------------------------------------------------------
State dispState_needToAckResetsMoving(
    [] {
      print_disp_state("...dispState_needToAckResetsMoving", DispState::names[(int)lastDispEvent]);
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
      print_disp_state("...dispState_boardVersionDoesntMatchScreen", DispState::names[(int)lastDispEvent]);
      screenBoardNotCompatible(board.packet.version);
    },
    NULL,
    NULL);
//---------------------------------------------------------------

State dispState_togglePushToStart(
    [] {
      print_disp_state("...dispState_togglePushToStart", DispState::names[(int)lastDispEvent]);
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
        displayState->trigger(DispState::PRIMARY_SINGLE_CLICK);
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
  // DispState::CONNECTED
  displayState->add_transition(&dispState_searching, &dispState_stoppedScreen, DispState::CONNECTED, NULL);
  displayState->add_transition(&dispState_disconnected, &dispState_stoppedScreen, DispState::CONNECTED, NULL);

  // DispState::DISCONNECTED
  displayState->add_transition(&dispState_stoppedScreen, &dispState_disconnected, DispState::DISCONNECTED, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_disconnected, DispState::DISCONNECTED, NULL);

  // DispState::SW_RESET
  displayState->add_transition(&dispState_stoppedScreen, &dispState_needToAckResetsStopped, DispState::SW_RESET, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_needToAckResetsMoving, DispState::SW_RESET, NULL);

  displayState->add_transition(&dispState_needToAckResetsStopped, &dispState_stoppedScreen, DispState::PRIMARY_DOUBLE_CLICK, acknowledgeResets);
  displayState->add_transition(&dispState_needToAckResetsStopped, &dispState_needToAckResetsMoving, DispState::MOVING, NULL);
  displayState->add_transition(&dispState_needToAckResetsMoving, &dispState_needToAckResetsStopped, DispState::STOPPED, NULL);
  displayState->add_transition(&dispState_needToAckResetsMoving, &dispState_movingScreen, DispState::PRIMARY_DOUBLE_CLICK, acknowledgeResets);

  // DispState::MOVING
  displayState->add_transition(&dispState_stoppedScreen, &dispState_movingScreen, DispState::MOVING, NULL);
  // DispState::STOPPED
  displayState->add_transition(&dispState_movingScreen, &dispState_stoppedScreen, DispState::STOPPED, NULL);

  displayState->add_transition(&dispState_stoppedScreen, &dispState_togglePushToStart, DispState::PRIMARY_TRIPLE_CLICK, NULL);
  displayState->add_transition(&dispState_togglePushToStart, &dispState_togglePushToStart, DispState::PRIMARY_DOUBLE_CLICK, NULL);
  displayState->add_transition(&dispState_togglePushToStart, &dispState_stoppedScreen, DispState::PRIMARY_SINGLE_CLICK, NULL);

  // DispState::UPDATE
  displayState->add_transition(&dispState_stoppedScreen, &dispState_stoppedScreen, DispState::UPDATE, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_movingScreen, DispState::UPDATE, NULL);

  // DispState::VERSION_DOESNT_MATCH
  displayState->add_transition(&dispState_stoppedScreen, &dispState_boardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
  displayState->add_transition(&dispState_movingScreen, &dispState_boardVersionDoesntMatchScreen, DispState::VERSION_DOESNT_MATCH, NULL);
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
  while (read_from_display_event_queue() != DispState::NO_EVENT)
  {
  }
}
//---------------------------------------------------------------