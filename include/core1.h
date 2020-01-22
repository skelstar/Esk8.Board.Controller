
bool deadman_held;
uint8_t raw_throttle, max_throttle = 127;
bool waiting_for_idle_throttle;

#ifndef Fsm
#include <Fsm.h>
#endif

#define READ_TRIGGER_PERIOD 200

// prototypes
void update_deadman();

TriggerLib trigger(/*pin*/ 13, /*deadzone*/ 10);

//-------------------------------------------------------
void read_trigger()
{
  update_deadman();
  uint8_t old_throttle = controller_packet.throttle;

  controller_packet.throttle = trigger.get_safe_throttle();

#ifdef PRINT_THROTTLE
  DEBUGVAL(controller_packet.throttle, deadman_held);
  if (old_throttle != controller_packet.throttle)
  {
    old_throttle = controller_packet.throttle;
  }
#endif
}

void update_deadman()
{
#ifdef USING_DEADMAN
  switch (read_from_deadman_event_queue())
  {
  case EV_DEADMAN_PRESSED:
    trigger.deadman_held = true;
    break;
  case EV_DEADMAN_RELEASED:
    trigger.deadman_held = false;
    break;
  }
#else
  deadman_held = true;
#endif
}
