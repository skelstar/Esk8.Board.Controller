

void encoderChanged(i2cEncoderLibV2 *obj)
{
  controller_packet.throttle = throttle.mapCounterToThrottle(_deadmanButton.isPressed());
  DEBUGVAL(obj->readCounterByte(), controller_packet.throttle);
}

void encoderButtonPushed(i2cEncoderLibV2 *obj)
{
  controller_packet.throttle = throttle.mapCounterToThrottle(_deadmanButton.isPressed());
  DEBUGVAL("button pushed!!!", controller_packet.throttle);
}

void deadmanReleased(Button2 &btn)
{
  controller_packet.throttle = throttle.mapCounterToThrottle(/*pressed*/ false);
  DEBUGVAL(controller_packet.throttle);
}

void init_throttle()
{
  // throttle
  Wire.begin();
  throttle.init(/*changed*/ encoderChanged,
                /*pushed*/ encoderButtonPushed,
                /*min*/ -ENCODER_NUM_STEPS_BRAKE,
                /*max*/ ENCODER_NUM_STEPS_ACCEL);
  _deadmanButton.setReleasedHandler(deadmanReleased);
}
