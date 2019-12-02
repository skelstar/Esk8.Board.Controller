
enum StateMachineEventEnum
{
  EV_BUTTON_CLICK,
  EV_SERVER_CONNECTED,
  EV_SERVER_DISCONNECTED,
  EV_MOVING,
  EV_STOPPED_MOVING,
  EV_HELD_DOWN_WAIT,
  EV_NO_HELD_OPTION_SELECTED,
} fsm_event;

//-------------------------------
State state_connecting(
  [] { 
    DEBUG("state_connecting");
  }, 
  NULL, 
  NULL
);
//-------------------------------
State state_connected(
  [] {
    DEBUG("state_connected");
    lcdMessage("connected");
  },
  NULL, 
  NULL
);
//-------------------------------
State state_disconnected(
  [] {
    DEBUG("state_disconnected");
    u8g2.clearBuffer();
    lcd_line_text(5, 64/2, "disconnected", /*vertical*/ true, /*horizontal*/ true);
    u8g2.sendBuffer();
  },
  NULL, 
  NULL
);
//-------------------------------
State state_ready(
  [] {
    DEBUG("state_ready");
    lcdMessage("ready");
  },
  NULL, 
  NULL
);
//-------------------------------

Fsm fsm(&state_connecting);

void addFsmTransitions() {

  fsm_event = EV_SERVER_DISCONNECTED;
  fsm.add_transition(&state_connected, &state_disconnected, fsm_event, NULL);
  fsm.add_transition(&state_ready, &state_disconnected, fsm_event, NULL);

  fsm_event = EV_SERVER_CONNECTED;
  fsm.add_transition(&state_connecting, &state_connected, fsm_event, NULL);
  fsm.add_timed_transition(&state_connected, &state_ready, 1000, NULL);
  fsm.add_transition(&state_disconnected, &state_connected, fsm_event, NULL);

  fsm_event = EV_BUTTON_CLICK;

  fsm_event = EV_MOVING;

  fsm_event = EV_STOPPED_MOVING;

  fsm_event = EV_HELD_DOWN_WAIT;

  fsm_event = EV_NO_HELD_OPTION_SELECTED;  // no option selected
}
/* ---------------------------------------------- */