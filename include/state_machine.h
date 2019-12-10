
enum StateMachineEventEnum
{
  EV_BUTTON_CLICK,
  EV_BOARD_CONNECTED,
  EV_MOVING,
  EV_STOPPED_MOVING,
  EV_HELD_DOWN_WAIT,
  EV_NO_HELD_OPTION_SELECTED,
  EV_RECV_PACKET,
  EV_BOARD_FIRST_CONNECT,
  EV_PACKET_MISSED,
  EV_BOARD_TIMEOUT, // havne't heard from the board for a while (BOARD_COMMS_TIMEOUT)
} fsm_event;


//prototypes
void handle_board_first_packet();

//-------------------------------
State state_connecting(
    [] {
      DEBUG("state_connecting ----------------------------------------");
      tft.fillScreen(TFT_BLACK);
      lcd_message(MC_DATUM, "searching..", TFT_WIDTH/2, TFT_HEIGHT/2, 2);
    },
    NULL,
    NULL);
//-------------------------------
State state_syncing(
    [] {
      DEBUG("state_syncing ----------------------------------------");
      tft.fillScreen(TFT_BLACK);
      lcd_message(MC_DATUM, "syncing..", TFT_WIDTH/2, TFT_HEIGHT/2, 2);
    },
    NULL,
    NULL);
//-------------------------------
State state_searching(
    [] {
      DEBUG("state_searching ----------------------------------------");
      tft.fillScreen(TFT_BLACK);
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
      sprintf(buff, "%3d", board.num_times_controller_offline);
      chunkyDrawFloat(MC_DATUM, buff, NULL, 5, 10);
      lcd_message(TC_DATUM, "Board missed", TFT_WIDTH/2, (TFT_HEIGHT/2) + FONT_1_HEIGHT, 1);
      // board restart count
      char buff1[20];
      sprintf(buff1, "Board reset: %d", board_first_packet_count);
      lcd_bottom_line(buff1);
    },
    NULL,    
    NULL);
//-------------------------------
State state_board_timedout(
    [] {
      DEBUG("state_board_timedout ----------------------------------------");
      controller_packet.throttle = 127; 
      tft.fillScreen(TFT_BLACK);
      lcd_message(MC_DATUM, "timed out!", TFT_WIDTH/2, TFT_HEIGHT/2, 2);
    },
    NULL,    
    NULL);
//-------------------------------
void handle_board_first_packet()
{
  board_first_packet_count++;
}

Fsm fsm(&state_connecting);

void addFsmTransitions()
{
  // fsm.add_transition(&state_connecting, &state_syncing, EV_BOARD_CONNECTED, NULL);
  fsm.add_transition(&state_connecting, &state_syncing, EV_RECV_PACKET, NULL);
  fsm.add_transition(&state_syncing, &state_missing_packets, EV_RECV_PACKET, NULL);

  fsm.add_transition(&state_missing_packets, &state_missing_packets, EV_PACKET_MISSED, NULL);
  fsm.add_transition(&state_missing_packets, &state_board_timedout, EV_BOARD_TIMEOUT, NULL);
  fsm.add_transition(&state_missing_packets, &state_missing_packets, EV_BOARD_FIRST_CONNECT, handle_board_first_packet);
  fsm.add_transition(&state_board_timedout, &state_missing_packets, EV_RECV_PACKET, NULL);
}
/* ---------------------------------------------- */