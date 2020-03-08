

// uint8_t brakeIn[] = {0, 30, 70, 127};
// uint8_t brakeOut[] = {0, 50, 90, 127};

// uint8_t accelIn[] = {127, 180, 200, 255};
// uint8_t accelOut[] = {127, 140, 170, 255};

void init_throttle()
{
}

void updateStatusPixel()
{
  if (controller_packet.throttle == 0)
  {
    sendToPixelEventQueue(PIXEL_THROTTLE_MIN);
  }
  else if (controller_packet.throttle < 127)
  {
    sendToPixelEventQueue(PIXEL_BRAKING);
  }
  else if (controller_packet.throttle == 255)
  {
    sendToPixelEventQueue(PIXEL_THROTTLE_MAX);
  }
  else if (controller_packet.throttle > 127)
  {
    sendToPixelEventQueue(PIXEL_ACCEL);
  }
  else
  {
    sendToPixelEventQueue(PIXEL_THROTTLE_IDLE);
  }
}