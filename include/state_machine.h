
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
  EV_REQUESTED_UPDATE,
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
      draw_missing_packets_screen();
    },
    NULL,    
    NULL);
//-------------------------------
State state_waiting_for_update(
    [] {
      since_requested_update = 0;
    },
    [] {
      if (since_requested_update > 1000) 
      {
        TRIGGER(EV_BOARD_TIMEOUT, "EV_BOARD_TIMEOUT");
      }
    },    
    NULL);
//-------------------------------
State state_board_timedout(
    [] {
      DEBUG("state_board_timedout ----------------------------------------");
      controller_packet.throttle = 127; 
      // assuming on missing_packet screen
      tft.fillRect(0, 0, TFT_WIDTH, FONT_2_HEIGHT, TFT_RED);
      tft.setTextColor(TFT_WHITE, TFT_RED);
      lcd_message(MC_DATUM, "timed out!", TFT_WIDTH/2, FONT_2_HEIGHT/2, 2);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    },
    NULL,    
    NULL);
//-------------------------------
  State state_show_battery(
    [] {
        DEBUG("state_show_battery ----------------------------------------");
    },
    NULL,
    NULL);
//-------------------------------
void handle_board_first_packet()
{
  board_first_packet_count++;
}
//-------------------------------

Fsm fsm(&state_connecting);

void addFsmTransitions()
{
  fsm.add_transition(&state_connecting, &state_syncing, EV_RECV_PACKET, NULL);
  fsm.add_transition(&state_syncing, &state_missing_packets, EV_RECV_PACKET, NULL);

  fsm.add_transition(&state_missing_packets, &state_missing_packets, EV_PACKET_MISSED, NULL);
  fsm.add_transition(&state_missing_packets, &state_missing_packets, EV_BOARD_FIRST_CONNECT, handle_board_first_packet);
  // requested update
  fsm.add_transition(&state_missing_packets, &state_waiting_for_update, EV_REQUESTED_UPDATE, NULL);
  fsm.add_transition(&state_waiting_for_update, &state_missing_packets, EV_RECV_PACKET, NULL);
  fsm.add_transition(&state_waiting_for_update, &state_board_timedout, EV_BOARD_TIMEOUT, NULL);
  fsm.add_transition(&state_board_timedout, &state_missing_packets, EV_RECV_PACKET, NULL);

  // button 0 pressed
  fsm.add_transition(&state_missing_packets, &state_show_battery, EV_BUTTON_CLICK, NULL);
  fsm.add_timed_transition(&state_show_battery, &state_missing_packets, 1500, NULL);
}
/* ---------------------------------------------- */