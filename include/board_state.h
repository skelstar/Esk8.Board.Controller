
enum BoardEvent
{
  EV_BD_REQUESTED,
  EV_BD_TIMEDOUT,
  EV_BD_RESPONDED
};

// prototypes
void PRINT_BD_FSM_STATE(const char *state_name);
void BD_TRIGGER(BoardEvent x, char *s);
void set_request_update_command();

//-------------------------------------------------------
State board_unknown(
    [] {
      PRINT_BD_FSM_STATE("BD: board_unknown.........");
    },
    NULL,
    NULL);
//-------------------------------------------------------
State board_idle(
    [] {
      PRINT_BD_FSM_STATE("BD: board_idle.........");
      // FSM_EVENT(EV_BOARD_CONNECTED, NULL);
    },
    NULL,    
    NULL);
//-------------------------------------------------------
State board_requested(
    [] {
      PRINT_BD_FSM_STATE("BD: board_requested.........");
    },
    NULL,
    NULL);
//-------------------------------------------------------
State board_timedout(
    [] {
      PRINT_BD_FSM_STATE("BD: board_timedout.........");
      FSM_EVENT(EV_BOARD_TIMEOUT, NULL);
    },
    NULL,
    NULL);
//-------------------------------------------------------

Fsm board_fsm(&board_unknown);

void add_board_fsm_transitions()
{
  board_fsm.add_transition(&board_unknown, &board_requested, EV_BD_REQUESTED, NULL);
  board_fsm.add_transition(&board_idle, &board_requested, EV_BD_REQUESTED, NULL);

  board_fsm.add_transition(&board_idle, &board_timedout, EV_BD_TIMEDOUT, NULL);
  board_fsm.add_transition(&board_requested, &board_timedout, EV_BD_TIMEDOUT, NULL);
  board_fsm.add_transition(&board_timedout, &board_unknown, EV_BD_TIMEDOUT, NULL);
  
  board_fsm.add_transition(&board_requested, &board_idle, EV_BD_RESPONDED, NULL);
  board_fsm.add_transition(&board_timedout, &board_requested, EV_BD_REQUESTED, NULL);
}

//-------------------------------------------------------

void set_request_update_command()
{
  // send request next packet
  controller_packet.command = COMMAND_REQUEST_UPDATE;
  BD_TRIGGER(EV_BD_REQUESTED, "EV_BD_REQUESTED");
}

void PRINT_BD_FSM_STATE(const char *state_name)
{
#ifdef PRINT_BOARD_FSM_STATE_NAME
  DEBUG(state_name);
#endif
}

void BD_TRIGGER(BoardEvent x, char *s)
{
  if (s != NULL)
  {
#ifdef PRINT_BOARD_FSM_EVENT
    Serial.printf("BD EVENT: %s (%lu)\n", s, millis());
#endif
  }
  board_fsm.trigger(x);
  board_fsm.run_machine();
}
