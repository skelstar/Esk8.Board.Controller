

enum BoardStateId
{
  BD_IDLE,
  BD_REQUESTED,
  BD_TIMED_OUT
};

enum BoardEvent
{
  EV_BD_REQUESTED,
  EV_BD_TIMEDOUT,
  EV_BD_RESPONDED
};

void PRINT_BOARD_STATE_NAME(const char *state_name);
void BD_TRIGGER(BoardEvent x, char *s);
void request_update();

elapsedMillis since_last_requested_update = 0;

//-------------------------------------------------------
State board_unknown(
    [] {
      PRINT_BOARD_STATE_NAME("BD: board_unknown.........");
      since_last_requested_update = 0;
    },
    [] {
      if (since_last_requested_update > REQUEST_FROM_BOARD_INITIAL_INTERVAL)
      {
        request_update();
      }
    },
    NULL);
//-------------------------------------------------------
State board_idle(
    [] {
      PRINT_BOARD_STATE_NAME("BD: board_idle.........");
      TRIGGER(EV_BOARD_CONNECTED, NULL);
      since_last_requested_update = 0;
    },
    [] {
      if (since_last_requested_update > REQUEST_FROM_BOARD_INTERVAL)
      {
        request_update();
      }
    },
    NULL);
//-------------------------------------------------------
State board_requested(
    [] {
      PRINT_BOARD_STATE_NAME("BD: board_requested.........");
      since_waiting_for_response = 0;
    },
    [] {
      if (since_waiting_for_response > BOARD_COMMS_TIMEOUT)
      {
        BD_TRIGGER(EV_BD_TIMEDOUT, "EV_BD_TIMEDOUT");
      }
    },
    NULL);
//-------------------------------------------------------
State board_timedout(
    [] {
      PRINT_BOARD_STATE_NAME("BD: board_timedout.........");
      TRIGGER(EV_BOARD_TIMEOUT, NULL);
    },
    NULL,
    NULL);
//-------------------------------------------------------

Fsm board_fsm(&board_unknown);

void add_board_fsm_transitions()
{
  board_fsm.add_transition(&board_unknown, &board_requested, EV_BD_REQUESTED, NULL);
  board_fsm.add_transition(&board_idle, &board_requested, EV_BD_REQUESTED, NULL);
  board_fsm.add_transition(&board_requested, &board_timedout, EV_BD_TIMEDOUT, NULL);
  board_fsm.add_transition(&board_timedout, &board_unknown, EV_BD_TIMEDOUT, NULL);
  board_fsm.add_transition(&board_requested, &board_idle, EV_BD_RESPONDED, NULL);
  board_fsm.add_transition(&board_timedout, &board_requested, EV_BD_REQUESTED, NULL);
}

//-------------------------------------------------------

void request_update()
{
  since_last_requested_update = 0;
  // send request next packet
  nrf24.controllerPacket.command = COMMAND_REQUEST_UPDATE;
  BD_TRIGGER(EV_BD_REQUESTED, "EV_BD_REQUESTED");
}

void PRINT_BOARD_STATE_NAME(const char *state_name)
{
#ifdef DEBUG_BOARD_PRINT_STATE_NAME
  DEBUG(state_name);
#endif
}

void BD_TRIGGER(BoardEvent x, char *s)
{
  if (s != NULL)
  {
#ifdef BOARD_FSM_TRIGGER_DEBUG_ENABLED
    Serial.printf("BD EVENT: %s (%lu)\n", s, millis());
#endif
  }
  board_fsm.trigger(x);
  board_fsm.run_machine();
}
