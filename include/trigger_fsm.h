

enum TriggerEvent
{
  TRIGGER_OK,
  TRIGGER_DEADMAN_RELEASED,
  TRIGGER_SAFE,
  TRIGGER_DEADMAN_PRESSED,
};

enum TriggerState
{
  TRIGGER_STATE_OK,
  TRIGGER_STATE_WAIT_NOT_ACCEL,
  TRIGGER_STATE_DM_RELEASED,  
};

bool trigger_updated;

void TRIGGER_TRIGGER(TriggerEvent event, char* s);

void print_trigger_state(char* state_name)
{
  #ifdef DEBUG_PRINT_TRIGGER_STATE
  DEBUG(state_name);
  #endif
}

State state_trigger_ok(
    TRIGGER_STATE_OK,
    [] {
      print_trigger_state("--> state_trigger_ok");
      can_accelerate = true;
      trigger_updated = true;
    },
    NULL,
    NULL);

State state_trigger_wait_not_accelerating(
    TRIGGER_STATE_WAIT_NOT_ACCEL,
    [] {
      print_trigger_state("--> state_trigger_wait_not_accelerating");
      can_accelerate = false;
      trigger_updated = true;
    },
    [] {
      bool is_safe = throttle_unfiltered <= 127;
      if (is_safe)
      {
        TRIGGER_TRIGGER(TRIGGER_SAFE, "TRIGGER_SAFE");
      }
    },
    NULL);

State state_trigger_deadman_released(
    TRIGGER_STATE_DM_RELEASED,
    [] {
      print_trigger_state("--> state_trigger_deadman_released");
      can_accelerate = false;
      trigger_updated = true;
    },
    NULL,
    NULL);

Fsm trigger_fsm(&state_trigger_deadman_released);

void addTriggerFsmTransitions()
{
  trigger_fsm.add_transition(&state_trigger_ok, &state_trigger_deadman_released, TRIGGER_DEADMAN_RELEASED, NULL);
  trigger_fsm.add_transition(&state_trigger_deadman_released, &state_trigger_wait_not_accelerating, TRIGGER_DEADMAN_PRESSED, NULL);
  trigger_fsm.add_transition(&state_trigger_wait_not_accelerating, &state_trigger_ok, TRIGGER_SAFE, NULL);
}

void TRIGGER_TRIGGER(TriggerEvent event, char *s)
{
  if (s != NULL)
  {
#ifdef PRINT_TRIGGER_FSM_EVENT
    Serial.printf("TRIGGER_FSM EVENT: %s\n", s);
#endif
  }
  trigger_fsm.trigger(event);
}
