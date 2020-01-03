
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
  STATE_MAIN_SCREEN,
  STATE_WAITING_FOR_UPDATE,
  STATE_BOARD_TIMEDOUT,
  STATE_SHOW_BATTERY,
};

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
State state_main_screen(
  STATE_MAIN_SCREEN,
  [] {
    DEBUG("state_main_screen --------");
    lcdMessage("Main");
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
void handle_board_first_packet()
{
  DEBUG("handle_board_first_packet");
  board_first_packet_count++;
}
//-------------------------------
void handle_stopped_moving()
{
  // last_trip.save(vescdata);
  DEBUG("saved vesc data");
}
//-------------------------------
void handle_last_will() 
{
  DEBUG("last will received");
}
//-------------------------------

Fsm fsm(&state_connecting);

void addFsmTransitions()
{
  // first connect
  fsm.add_transition(&state_connecting, &state_main_screen, EV_RECV_PACKET, NULL);
  fsm.add_transition(&state_connecting, &state_main_screen, EV_BOARD_FIRST_CONNECT, handle_board_first_packet);
  fsm.add_transition(&state_main_screen, &state_main_screen, EV_BOARD_FIRST_CONNECT, handle_board_first_packet);
  // requested update
  // fsm.add_transition(&state_main_screen, &state_waiting_for_update, EV_REQUESTED_UPDATE, NULL);
  // fsm.add_transition(&state_waiting_for_update, &state_main_screen, EV_REQUESTED_RESPONSE, NULL);
  // fsm.add_transition(&state_waiting_for_update, &state_board_timedout, EV_BOARD_TIMEOUT, NULL);
  // timed out
  // fsm.add_transition(&state_board_timedout, &state_main_screen, EV_RECV_PACKET, NULL);
  // fsm.add_transition(&state_board_timedout, &state_main_screen, EV_BOARD_FIRST_CONNECT, NULL);
  // fsm.add_transition(&state_board_timedout, &state_main_screen, EV_REQUESTED_RESPONSE, NULL);
  
  fsm.add_transition(&state_main_screen, &state_main_screen, EV_STOPPED_MOVING, handle_stopped_moving);
  fsm.add_transition(&state_main_screen, &state_main_screen, EV_STARTED_MOVING, NULL);

  // last will
  // fsm.add_transition(&state_main_screen, &state_main_screen, EV_BOARD_LAST_WILL, handle_last_will);

  // button 0 pressed
  fsm.add_transition(&state_main_screen, &state_show_battery, EV_BUTTON_CLICK, NULL);
  fsm.add_timed_transition(&state_show_battery, &state_main_screen, 1500, NULL);
}
/* ---------------------------------------------- */
uint8_t get_prev_state()
{
  return fsm.get_from_state();
}
