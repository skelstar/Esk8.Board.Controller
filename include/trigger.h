#ifndef Arduino_h
#include <Arduino.h>
#endif

uint16_t trigger_centre = 0;
uint16_t trigger_max = 0;
uint16_t trigger_min = 0;
bool trigger_calibrated = false;

uint8_t deadzone = 10; // either side of trigger_centre

uint16_t get_trigger_raw()
{
  return analogRead(13);
}

uint8_t get_mapped_calibrated_throttle()
{
  if (trigger_calibrated == false)
  {
    return 127;
  }

  uint16_t raw = get_trigger_raw();

  uint16_t centre_upper = trigger_centre + deadzone;
  uint16_t centre_lower = trigger_centre - deadzone;

  if (raw > centre_upper)
  {
    return map(raw, centre_upper, 4096, 127, 255);
  }
  else if (raw < centre_lower)
  {
    return map(raw, 0, centre_lower, 0, 127);
  }
  return 127;
}

void update_throttle()
{
  uint8_t old_throttle = controller_packet.throttle;

  throttle_unfiltered = get_mapped_calibrated_throttle();

#ifndef USE_DEADMAN_SWITCH
  can_accelerate = true;
#endif

  // check for safety/conditions
  controller_packet.throttle = throttle_unfiltered;
  if (can_accelerate == false && throttle_unfiltered > 127)
  {
    controller_packet.throttle = 127;
  }

#ifdef TRIGGER_DEBUG_ENABLED
  bool trigger_changed = old_throttle != controller_packet.throttle;
  if (trigger_changed)
  {
    DEBUGVAL(controller_packet.throttle);
  }
#endif
}

elapsedMillis since_last_read_trigger = 0;

void read_trigger()
{
  if (since_last_read_trigger > READ_TRIGGER_INTERVAL)
  {
    since_last_read_trigger = 0;
    if (xControllerPacketSemaphore != NULL && xSemaphoreTake(xControllerPacketSemaphore, (TickType_t)10) == pdTRUE)
    {
      update_throttle();
      xSemaphoreGive(xControllerPacketSemaphore);
    }
    else
    {
      DEBUG("Can't take semaphore!");
    }
  }
}
