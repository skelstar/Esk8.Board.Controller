
enum StateMachineEventEnum
{
  EV_BUTTON_CLICK,
  EV_BOARD_CONNECTED,
  EV_MOVING,
  EV_STOPPED_MOVING,
  EV_HELD_DOWN_WAIT,
  EV_NO_HELD_OPTION_SELECTED,
  EV_RECV_PACKET,
  EV_PACKET_MISSED,
  EV_BOARD_TIMEOUT, // havne't heard from the board for a while (BOARD_COMMS_TIMEOUT)
} fsm_event;

//-------------------------------
State state_connecting(
    [] {
      DEBUG("state_connecting ----------------------------------------");
      lcdMessage("searching..");
    },
    NULL,
    NULL);
//-------------------------------
State state_syncing(
    [] {
      DEBUG("state_syncing ----------------------------------------");
      lcdMessage("syncing");
    },
    NULL,
    NULL);
//-------------------------------
State state_searching(
    [] {
      DEBUG("state_searching ----------------------------------------");
      lcdMessage("connected");
      missedPacketCounter = 0;
    },
    NULL,
    NULL);
//-------------------------------
State state_missing_packets(
    [] {
      DEBUG("state_missing_packets ----------------------------------------");
      lcdMessage("ready");

      if (board.num_times_controller_offline > 0)
      {
        char buff[20];
        sprintf(buff, "missed: %d", board.num_times_controller_offline);
        lcd_bottom_line(buff);
      }
    },
    NULL,    
    NULL);
//-------------------------------
State state_board_timedout(
    [] {
      DEBUG("state_board_timedout ----------------------------------------");
      controller_packet.throttle = 127; 
      lcdMessage("TIMED OUT");
    },
    NULL,    
    NULL);
//-------------------------------

Fsm fsm(&state_connecting);

void addFsmTransitions()
{
  // fsm.add_transition(&state_connecting, &state_syncing, EV_BOARD_CONNECTED, NULL);
  fsm.add_transition(&state_connecting, &state_syncing, EV_RECV_PACKET, NULL);
  fsm.add_transition(&state_syncing, &state_missing_packets, EV_RECV_PACKET, NULL);

  fsm.add_transition(&state_missing_packets, &state_missing_packets, EV_PACKET_MISSED, NULL);
  fsm.add_transition(&state_missing_packets, &state_board_timedout, EV_BOARD_TIMEOUT, NULL);
  fsm.add_transition(&state_board_timedout, &state_missing_packets, EV_RECV_PACKET, NULL);
}
/* ---------------------------------------------- */