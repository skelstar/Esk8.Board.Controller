

void encoderChanged(i2cEncoderLibV2 *obj)
{
  uint8_t old_throttle = controller_packet.throttle;
  controller_packet.throttle = throttle.mapCounterToThrottle(_deadmanButton.isPressed());

#ifdef PRINT_THROTTLE
  if (old_throttle != controller_packet.throttle)
  {
    DEBUGVAL("encoderChanged", controller_packet.throttle);
  }
#endif
}

void encoderButtonPushed(i2cEncoderLibV2 *obj)
{
  DEBUGVAL("button pushed!!!");
}

void encoderButtonDoubleClicked(i2cEncoderLibV2 *obj)
{
  DEBUGVAL("button double clicked!!!");
}

void deadmanReleased(Button2 &btn)
{
  if (controller_packet.throttle < 127)
  {
    controller_packet.throttle = 127;
    throttle.clear();
  }
  controller_packet.throttle = throttle.mapCounterToThrottle(/*pressed*/ false);
  DEBUGVAL(controller_packet.throttle);
}

void init_throttle()
{
  Wire.begin();
  throttle.init(/*changed*/ encoderChanged,
                /*pushed*/ encoderButtonPushed,
                /*double*/ encoderButtonDoubleClicked,
                /*min*/ -ENCODER_NUM_STEPS_BRAKE,
                /*max*/ ENCODER_NUM_STEPS_ACCEL);
  throttle.setMap(GENTLE);
  _deadmanButton.setReleasedHandler(deadmanReleased);
}
