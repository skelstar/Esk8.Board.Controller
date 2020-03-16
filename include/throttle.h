

void encoderButtonPushed(i2cEncoderLibV2 *obj)
{
  DEBUG("Encoder button pushed");
  throttle.clear();
}

void encoderButtonDoublePushed(i2cEncoderLibV2 *obj)
{
  DEBUG("Encoder button double-pushed");
  switch ((int)throttle.getMap())
  {
  case ThrottleMap::LINEAR:
    throttle.setMap(GENTLE);
    throttle.clear();
    DEBUG("ThrottleMap::GENTLE");
    break;
  case ThrottleMap::GENTLE:
    throttle.setMap(SMOOTHED);
    throttle.clear();
    DEBUG("ThrottleMap::SMOOTHED");
    break;
  case ThrottleMap::SMOOTHED:
    throttle.setMap(LINEAR);
    throttle.clear();
    DEBUG("ThrottleMap::LINEAR");
    break;
  default:
    DEBUG("DEFAULT");
    break;
  }
}

void init_throttle()
{
  Wire.begin();
  throttle.init(
      encoderButtonPushed,
      encoderButtonDoublePushed,
      /*min counts*/ -ENCODER_BRAKE_COUNTS,
      /*max counts*/ ENCODER_ACCEL_COUNTS);

  throttle.setSmoothBufferLengths(/*brake*/ 3, /*accel*/ 3);
  throttle.setMap(LINEAR);
}

void updateStatusPixel()
{
  if (controller_packet.throttle == 0)
  {
    // sendToPixelEventQueue(PIXEL_THROTTLE_MIN);
  }
  else if (controller_packet.throttle < 127)
  {
    // sendToPixelEventQueue(PIXEL_BRAKING);
  }
  else if (controller_packet.throttle == 255)
  {
    // sendToPixelEventQueue(PIXEL_THROTTLE_MAX);
  }
  else if (controller_packet.throttle > 127)
  {
    // sendToPixelEventQueue(PIXEL_ACCEL);
  }
  else
  {
    // sendToPixelEventQueue(PIXEL_THROTTLE_IDLE);
  }
}