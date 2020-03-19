

void encoderButtonPushed(i2cEncoderLibV2 *obj)
{
  DEBUG("Encoder button pushed");
  throttle.clear();
  send_to_display_event_queue(DISP_EV_OPTION_SELECT_VALUE);
}

void encoderButtonDoublePushed(i2cEncoderLibV2 *obj)
{
  DEBUG("Encoder button double-pushed");
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
  }
  else if (controller_packet.throttle < 127)
  {
  }
  else if (controller_packet.throttle == 255)
  {
  }
  else if (controller_packet.throttle > 127)
  {
  }
  else
  {
  }
}