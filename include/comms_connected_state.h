#ifndef Fsm
#include <Fsm.h>
#endif

enum CommsStateEvent
{
  EV_COMMS_CONNECTED,
  EV_COMMS_DISCONNECTED,
};

bool comms_session_started;
bool comms_state_connected;

void print_state(const char *state_name);

State _state_searching(
    [] {
      print_state("..._state_searching");
      comms_session_started = false;
      comms_state_connected = false;
    },
    NULL,
    [] {
      display_state_event(DISP_EV_CONNECTED);
    });

State _state_connected(
    [] {
      print_state("..._state_connected");
      comms_session_started = true;
      comms_state_connected = true;
    },
    NULL,
    [] {
      display_state_event(DISP_EV_CONNECTED);
    });

State _state_disconnected(
    [] {
      print_state("..._state_disconnected");
      display_state_event(DISP_EV_DISCONNECTED);
      comms_state_connected = false;
    },
    NULL,
    NULL);

Fsm comms_state_fsm(&_state_searching);

void add_comms_state_transitions()
{
  comms_state_fsm.add_transition(&_state_searching, &_state_connected, EV_COMMS_CONNECTED, NULL);
  comms_state_fsm.add_transition(&_state_connected, &_state_disconnected, EV_COMMS_DISCONNECTED, NULL);
  comms_state_fsm.add_transition(&_state_disconnected, &_state_connected, EV_COMMS_CONNECTED, NULL);
}

void print_state(const char *state_name)
{
#ifdef PRINT_COMMS_STATE
  DEBUG(state_name);
#endif
}

void comms_state_event(CommsStateEvent ev)
{
#ifdef PRINT_COMMS_STATE_EVENT
  if (ev == EV_COMMS_CONNECTED && !comms_state_connected)
  {
    DEBUG("-->Comms State: Connected");
  }
  else if (ev == EV_COMMS_DISCONNECTED && comms_state_connected)
  {
    DEBUG("-->Comms State: Disconnected!");
  }
  else
  {
  }
#endif
  comms_state_fsm.trigger(ev);
}