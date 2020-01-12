

enum TriggerEvent
{
  TRIGGER_OK,
  TRIGGER_DEADMAN_RELEASED,
  TRIGGER_SAFE,
  TRIGGER_DEADMAN_PRESSED,
};

void TRIGGER_TRIGGER(TriggerEvent event);

void print_trigger_state(char* state_name)
{
  #ifdef DEBUG_PRINT_TRIGGER_STATE
  DEBUG(state_name);
  #endif
}

State state_trigger_ok(
    [] {
      print_trigger_state("--> state_trigger_ok");
      can_accelerate = true;
    },
    NULL,
    NULL);

State state_trigger_wait_not_accelerating(
    [] {
      print_trigger_state("--> state_trigger_wait_not_accelerating");
      can_accelerate = false;
    },
    [] {
      bool is_safe = throttle_unfiltered <= 127;
      if (is_safe)
      {
        TRIGGER_TRIGGER(TRIGGER_SAFE);
      }
    },
    NULL);

State state_trigger_deadman_released(
    [] {
      print_trigger_state("--> state_trigger_deadman_released");
      can_accelerate = false;
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

void TRIGGER_TRIGGER(TriggerEvent event)
{
  trigger_fsm.trigger(event);
}
