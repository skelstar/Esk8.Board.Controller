
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

//-----------------------------------------------------
uint8_t make_throttle_safe(uint8_t raw, bool deadman_held)
{
  bool braking_or_idle = raw <= 127;
  uint8_t result = 127;

  if (waiting_for_idle_throttle && braking_or_idle)
  {
    waiting_for_idle_throttle = false;
  }

  if (deadman_held)
  {
    result = raw;
    if (waiting_for_idle_throttle)
    {
      result = raw <= max_throttle
                   ? raw
                   : max_throttle;
      max_throttle = raw;
    }
  }
  else
  {
    result = braking_or_idle ? raw
                             : 127;
  }
  return result;
}

//-------------------------------------------------------
void read_trigger()
{
  update_deadman();
  uint8_t old_throttle = controller_packet.throttle;

  raw_throttle = trigger.get_throttle();
#ifdef USE_DEADMAN
  controller_packet.throttle = make_throttle_safe(raw_throttle);
#else
  controller_packet.throttle = raw_throttle;
#endif

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
    // DEBUG("pressed");
    deadman_held = true;
    break;
  case EV_DEADMAN_RELEASED:
    max_throttle = controller_packet.throttle;
    deadman_held = false;
    waiting_for_idle_throttle = true;
    // DEBUGVAL("released", deadman_held, waiting_for_idle_throttle, max_throttle);
    break;
  }
#else
  deadman = true;
#endif
}
