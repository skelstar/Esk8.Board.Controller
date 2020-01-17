
#include <Fsm.h>

#ifndef VescData
#include <VescData.h>
#endif

enum CommsEvent
{
  EV_COMMS_REQUESTED,
  EV_COMMS_TIMEDOUT,
  EV_COMMS_RESPONDED
};

// prototypes
void PRINT_COMMS_STATE(const char *state_name);
void COMMS_TRIGGER(CommsEvent x, char *s);

elapsedMillis since_requested_response;
//-------------------------------------------------------
State comms_normal(
    [] {
      PRINT_COMMS_STATE("COMMS: comms_normal.........");
    },
    [] {
      if (since_requested_response > 3000)
      {
        since_requested_response = 0;
        controller_packet.command = 1; // REQUEST
        COMMS_TRIGGER(EV_COMMS_REQUESTED, "EV_COMMS_REQUESTED");
      }
    },
    NULL);
//-------------------------------------------------------
State comms_requested(
    [] {
      PRINT_COMMS_STATE("COMMS: comms_requested.........");
    },
    [] {
      if (board_packet.reason == ReasonType::REQUESTED)
      {
        COMMS_TRIGGER(EV_COMMS_RESPONDED, "EV_COMMS_RESPONDED");
      }
    },
    NULL);
//-------------------------------------------------------
State comms_timedout(
    [] {
      PRINT_COMMS_STATE("COMMS: comms_timedout.........");
    },
    NULL,
    NULL);
//-------------------------------------------------------

Fsm comms_fsm(&comms_normal);

void add_comms_fsm_transitions()
{
  comms_fsm.add_transition(&comms_normal, &comms_requested, EV_COMMS_REQUESTED, NULL);

  comms_fsm.add_transition(&comms_requested, &comms_normal, EV_COMMS_RESPONDED, NULL);
  comms_fsm.add_transition(&comms_requested, &comms_timedout, EV_COMMS_TIMEDOUT, NULL);
  comms_fsm.add_transition(&comms_timedout, &comms_normal, EV_COMMS_RESPONDED, NULL);
}

//-------------------------------------------------------
void PRINT_COMMS_STATE(const char *state_name)
{
#ifdef PRINT_comms_FSM_STATE_NAME
  DEBUG(state_name);
#endif
}

void COMMS_TRIGGER(CommsEvent x, char *s)
{
  if (s != NULL)
  {
// #ifdef PRINT_comms_FSM_EVENT
    Serial.printf("COMMS EVENT: %s \n", s);
// #endif
  }
  comms_fsm.trigger(x);
  comms_fsm.run_machine();
}
