
#include <Fsm.h>

#ifndef VescData
#include <VescData.h>
#endif

enum CommsEvent
{
  EV_COMMS_REQUESTED,
  EV_COMMS_TIMEDOUT,
  EV_COMMS_RESPONDED,
  EV_COMMS_PACKET,
  EV_COMMS_FIRST_PACKET,
};

// prototypes
void PRINT_COMMS_STATE(const char *state_name);
void COMMS_EVENT(CommsEvent x, char *s);

elapsedMillis since_sent_request;
//-------------------------------------------------------
State comms_normal(
    [] {
      PRINT_COMMS_STATE("COMMS: comms_normal.........");
    },
    [] {
      if (since_sent_request > 5000)
      {
        since_sent_request = 0;
        controller_packet.command = 1; // REQUEST
        COMMS_EVENT(EV_COMMS_REQUESTED, "EV_COMMS_REQUESTED");
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
        controller_packet.command = 0;
        COMMS_EVENT(EV_COMMS_RESPONDED, "EV_COMMS_RESPONDED");
      }
      if (board_packet.id == 0)
      {
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
State comms_first_packet(
    [] {
      PRINT_COMMS_STATE("COMMS: comms_first_packet.........");
    },
    NULL,
    NULL);
//-------------------------------------------------------

Fsm comms_fsm(&comms_normal);

void add_comms_fsm_transitions()
{
  comms_fsm.add_transition(&comms_normal, &comms_requested, EV_COMMS_REQUESTED, NULL);

  comms_fsm.add_transition(&comms_requested, &comms_normal, EV_COMMS_RESPONDED, [] { DEBUGVAL(board_packet.id); });
  comms_fsm.add_transition(&comms_requested, &comms_timedout, EV_COMMS_TIMEDOUT, NULL);
  comms_fsm.add_transition(&comms_timedout, &comms_normal, EV_COMMS_RESPONDED, NULL);
  // first packet
  comms_fsm.add_transition(&comms_normal, &comms_first_packet, EV_COMMS_FIRST_PACKET, NULL);
  comms_fsm.add_transition(&comms_first_packet, &comms_normal, EV_COMMS_PACKET, NULL);
}

//-------------------------------------------------------
void PRINT_COMMS_STATE(const char *state_name)
{
#ifdef PRINT_COMMS_FSM_STATE_NAME
  DEBUG(state_name);
#endif
}

void COMMS_EVENT(CommsEvent x, char *s)
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
