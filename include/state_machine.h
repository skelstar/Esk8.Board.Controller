
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
  },
  [] {

  }, 
  NULL
);
//-------------------------------
State state_server_disconnected(
  [] {
    DEBUG("state_server_disconnected");
  },
  NULL, 
  NULL
);
//-------------------------------

Fsm fsm(&state_connecting);

void addFsmTransitions() {

  fsm_event = EV_SERVER_DISCONNECTED;
  fsm.add_transition(&state_connected, &state_server_disconnected, fsm_event, NULL);

  fsm_event = EV_SERVER_CONNECTED;
  fsm.add_transition(&state_connecting, &state_connected, fsm_event, NULL);
  fsm.add_transition(&state_server_disconnected, &state_connected, fsm_event, NULL);

  fsm_event = EV_BUTTON_CLICK;

  fsm_event = EV_MOVING;

  fsm_event = EV_STOPPED_MOVING;

  fsm_event = EV_HELD_DOWN_WAIT;

  fsm_event = EV_NO_HELD_OPTION_SELECTED;  // no option selected
}
/* ---------------------------------------------- */