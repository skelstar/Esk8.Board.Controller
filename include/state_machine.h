
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
      lcd_message(MC_DATUM, "searching..", TFT_WIDTH/2, TFT_HEIGHT/2, 2);
    },
    NULL,
    NULL);
//-------------------------------
State state_syncing(
    [] {
      DEBUG("state_syncing ----------------------------------------");
      lcd_message(MC_DATUM, "syncing..", TFT_WIDTH/2, TFT_HEIGHT/2, 2);
    },
    NULL,
    NULL);
//-------------------------------
State state_searching(
    [] {
      DEBUG("state_searching ----------------------------------------");
      lcd_message(MC_DATUM, "connected", TFT_WIDTH/2, TFT_HEIGHT/2, 2);
      missedPacketCounter = 0;
    },
    NULL,
    NULL);
//-------------------------------
State state_missing_packets(
    [] {
      DEBUG("state_missing_packets ----------------------------------------");
      tft.fillScreen(TFT_BLACK);
      char buff[4];
      sprintf(buff, "%03d", board.num_times_controller_offline);
      DEBUGVAL(buff);
      chunkyDrawFloat(MC_DATUM, buff, NULL, 5, 10);
      lcd_message(TC_DATUM, "board missed", TFT_WIDTH/2, (TFT_HEIGHT/2) + 25, 1);
    },
    NULL,    
    NULL);
//-------------------------------
State state_board_timedout(
    [] {
      DEBUG("state_board_timedout ----------------------------------------");
      controller_packet.throttle = 127; 
      lcd_message(MC_DATUM, "timed out!", TFT_WIDTH/2, TFT_HEIGHT/2, 2);
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