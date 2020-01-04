
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

uint8_t get_prev_state();
void TRIGGER(StateMachineEventEnum x, char *s);


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
    lcdMessage("Disconnected");
  },
  NULL,
  NULL);

//-------------------------------
State state_not_moving(
  STATE_NOT_MOVING,
  [] {
    DEBUG("state_not_moving --------");
    lcdMessage("Stopped");
  },
  NULL,
  NULL);
//-------------------------------
State state_moving(
  STATE_MOVING,
  [] {
    DEBUG("state_moving --------");
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

Fsm fsm(&state_connecting);

void addFsmTransitions()
{
  // state_connecting ->
  fsm.add_transition(&state_connecting, &state_not_moving, EV_BOARD_CONNECTED, NULL);
  fsm.add_transition(&state_connecting, &state_not_moving, EV_STOPPED_MOVING, NULL);
  fsm.add_transition(&state_connecting, &state_moving, EV_STARTED_MOVING, NULL);

  // -> state_disconnected
  fsm.add_transition(&state_not_moving, &state_disconnected, EV_BOARD_TIMEOUT, NULL);
  fsm.add_transition(&state_moving, &state_disconnected, EV_BOARD_TIMEOUT, NULL);

  // state_disconnected ->
  fsm.add_transition(&state_disconnected, &state_not_moving, EV_BOARD_CONNECTED, NULL);
  fsm.add_transition(&state_disconnected, &state_moving, EV_STARTED_MOVING, NULL);
  fsm.add_transition(&state_disconnected, &state_not_moving, EV_STOPPED_MOVING, NULL);

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

void TRIGGER(StateMachineEventEnum x, char *s)
{
  if (s != NULL)
  {
    Serial.printf("EVENT: %s\n", s);
  }
  fsm.trigger(x);
}


/* ---------------------------------------------- */
/* ---------------------------------------------- */
/* ---------------------------------------------- */
