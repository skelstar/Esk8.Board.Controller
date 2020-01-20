
bool deadman_pressed;

TriggerLib trigger(/*pin*/ 13, /*deadzone*/ 10);

void read_trigger()
{
  update_deadman();
  uint8_t old_throttle = controller_packet.throttle;

  controller_packet.throttle = trigger.get_throttle();

#ifdef PRINT_THROTTLE
  if (old_throttle != controller_packet.throttle)
  {
    DEBUGVAL(controller_packet.throttle);
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
    DEBUG("pressed");
    break;
  case EV_DEADMAN_RELEASED:
    DEBUG("released");
    break;
  }
#else
  deadman = true;
#endif
}
