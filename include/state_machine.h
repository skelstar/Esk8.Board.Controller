
enum StateMachineEventEnum
{
  EV_BUTTON_CLICK,
  EV_BOARD_CONNECTED,
  EV_STARTED_MOVING,
  EV_STOPPED_MOVING,
  EV_HELD_DOWN_WAIT,
  EV_NO_HELD_OPTION_SELECTED,
  EV_RECV_PACKET,
  EV_BOARD_FIRST_CONNECT,
  EV_PACKET_MISSED,
  EV_REQUESTED_UPDATE,
  EV_REQUESTED_RESPONSE,
  EV_BOARD_TIMEOUT, // havne't heard from the board for a while (BOARD_COMMS_TIMEOUT)
  EV_BOARD_LAST_WILL,
} fsm_event;

enum StateId
{
  STATE_CONNECTING,
  STATE_DISCONNECTED,
  STATE_NOT_MOVING,
  STATE_MOVING,
  STATE_WAITING_FOR_UPDATE,
  STATE_BOARD_TIMEDOUT,
  STATE_SHOW_BATTERY,
};

bool board_online = false;
bool board_has_disconnected = false;

uint8_t get_prev_state();

//-------------------------------
State state_connecting(
  STATE_CONNECTING,
  [] {
    DEBUG("state_connecting --------");
    lcdMessage("Connecting");
  },
  NULL,
  NULL);
//-------------------------------
State state_disconnected(
  STATE_DISCONNECTED,
  [] {
    DEBUG("state_disconnected --------");
    board_online = false;
    board_has_disconnected = true;
    lcdMessage("Disconnected");
  },
  NULL,
  NULL);

//-------------------------------
State state_not_moving(
  STATE_NOT_MOVING,
  [] {
    DEBUG("state_not_moving --------");
    board_online = true;
    lcdMessage("Stopped");
  },
  NULL,
  NULL);
//-------------------------------
State state_moving(
  STATE_MOVING,
  [] {
    DEBUG("state_moving --------");
    board_online = true;
    lcdMessage("Moving");
  },
  NULL,
  NULL);
//-------------------------------
State state_show_battery(
  STATE_SHOW_BATTERY,
  [] {
    DEBUG("state_show_battery --------");
    drawBattery(getBatteryPercentage(nrf24.boardPacket.batteryVoltage), true);
  },
  NULL,
  NULL);
//-------------------------------
void handle_last_will() 
{
  DEBUG("last will received");
}
//-------------------------------
void handle_from_disconnected()
{
  board_online = true;
  board_has_disconnected = false;
}
//-------------------------------
void handle_from_connecting()
{
  board_online = true;
  board_has_disconnected = false;
}
//-------------------------------

Fsm fsm(&state_connecting);

void addFsmTransitions()
{
  // state_connecting ->
  fsm.add_transition(&state_connecting, &state_not_moving, EV_RECV_PACKET, handle_from_connecting);
  fsm.add_transition(&state_connecting, &state_not_moving, EV_STOPPED_MOVING, handle_from_connecting);
  fsm.add_transition(&state_connecting, &state_moving, EV_STARTED_MOVING, handle_from_connecting);

  // -> state_disconnected
  fsm.add_transition(&state_not_moving, &state_disconnected, EV_BOARD_TIMEOUT, NULL);
  fsm.add_transition(&state_moving, &state_disconnected, EV_BOARD_TIMEOUT, NULL);

  // state_disconnected ->
  fsm.add_transition(&state_disconnected, &state_moving, EV_STARTED_MOVING, handle_from_disconnected);
  fsm.add_transition(&state_disconnected, &state_not_moving, EV_STOPPED_MOVING, handle_from_disconnected);

  // stopped -> moving -> stopped
  fsm.add_transition(&state_not_moving, &state_moving, EV_STARTED_MOVING, NULL);
  fsm.add_transition(&state_moving, &state_not_moving, EV_STOPPED_MOVING, NULL);

  // last will
  // fsm.add_transition(&state_main_screen, &state_main_screen, EV_BOARD_LAST_WILL, handle_last_will);

  // button 0 pressed
  fsm.add_transition(&state_not_moving, &state_show_battery, EV_BUTTON_CLICK, NULL);
  fsm.add_timed_transition(&state_show_battery, &state_not_moving, 1500, NULL);
}
/* ---------------------------------------------- */
uint8_t get_prev_state()
{
  return fsm.get_from_state();
}

/* ---------------------------------------------- */
/* ---------------------------------------------- */
/* ---------------------------------------------- */

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

void BD_TRIGGER(uint8_t x, char *s);
void request_update();

elapsedMillis since_last_requested_update = 0;


//-------------------------------------------------------
State board_unknown(
  []{
    DEBUG("BD: board_unknown.........");
    since_last_requested_update = 0;
  }, 
  [] {
    if (since_last_requested_update > 500)
    {
      request_update();
    }
  }, 
  NULL);
//-------------------------------------------------------
State board_idle(
  []{
    DEBUG("BD: board_idle.........");
    since_last_requested_update = 0;
  }, 
  [] {
    if (since_last_requested_update > 5000)
    {
      request_update();
    }
  }, 
  NULL);
//-------------------------------------------------------
State board_requested(
  [] {
    DEBUG("BD: board_requested.........");
    since_waiting_for_response = 0;
  },
  [] 
  {
    if (since_waiting_for_response > BOARD_COMMS_TIMEOUT)
    {
      BD_TRIGGER(EV_BD_TIMEDOUT, "EV_BD_TIMEDOUT");
    }
  }, 
  NULL);
//-------------------------------------------------------
State board_timedout(
  [] {
    DEBUG("BD: board_timedout.........");
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
  board_fsm.add_transition(&board_requested, &board_idle, EV_BD_RESPONDED, NULL);
  board_fsm.add_transition(&board_timedout, &board_idle, EV_BD_RESPONDED, NULL);
}

//-------------------------------------------------------

void request_update()
{
  since_last_requested_update = 0;
  // send request next packet
  nrf24.controllerPacket.command = COMMAND_REQUEST_UPDATE;
  BD_TRIGGER(EV_BD_REQUESTED, "EV_BD_REQUESTED");
}

void BD_TRIGGER(uint8_t x, char *s)
{
  if (s != NULL)
  {
    Serial.printf("BD EVENT: %s\n", s);
  }
  board_fsm.trigger(x);
  board_fsm.run_machine();
}
