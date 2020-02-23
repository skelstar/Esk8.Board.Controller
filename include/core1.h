
uint8_t raw_throttle, max_throttle = 127;
bool waiting_for_idle_throttle;

// prototypes

//-------------------------------------------------------
void read_trigger()
{
  uint8_t old_throttle = controller_packet.throttle;

#ifdef FEATURE_PUSH_TO_ENABLE
  controller_packet.throttle = throttle.getThrottle(board_packet.moving);
#else
  // controller_packet.throttle = throttle.getThrottle(true);
#endif

#ifdef PRINT_THROTTLE
  DEBUGVAL(controller_packet.throttle);
  if (old_throttle != controller_packet.throttle)
  {
    old_throttle = controller_packet.throttle;
  }
#endif
}
