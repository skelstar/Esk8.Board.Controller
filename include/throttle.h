
// void encoderChanged(i2cEncoderLibV2 *obj)
// {
//   uint8_t old_throttle = controller_packet.throttle;

//   if (throttle.getMap() == ThrottleMap::SMOOTHED)
//   {
//     // smoothedThrottle.add(throttle.mapCounterToThrottle());
//     // controller_packet.throttle = (uint8_t)smoothedThrottle.get();
//     // DEBUGVAL(controller_packet.throttle, smoothedThrottle.get());
//   }
//   else
//   {
//     controller_packet.throttle = throttle.mapCounterToThrottle();
//   }

// #ifdef PRINT_THROTTLE
//   if (old_throttle != controller_packet.throttle)
//   {
//     DEBUGVAL("encoderChanged", controller_packet.throttle);
//   }
// #endif
// }

// void onDeadmanChanged(i2cEncoderLibV2 *obj)
// {
//   throttle._deadmanHeld = obj->readGP2() == 0;
//   if (throttle._deadmanHeld == false && controller_packet.throttle < 127)
//   {
//     throttle.resetCounter();
//   }
//   encoderChanged(obj);
// #ifdef PRINT_THROTTLE
//   DEBUGVAL(throttle._deadmanHeld);
// #endif
// }

// void encoderButtonPushed(i2cEncoderLibV2 *obj)
// {
//   DEBUGVAL("button pushed!!!");
// }

// void encoderButtonDoubleClicked(i2cEncoderLibV2 *obj)
// {
//   DEBUGVAL("button double clicked!!!");
// }

void init_throttle()
{
  // Wire.begin();
  // throttle.init(/*changed*/ encoderChanged,
  //               /*pushed*/ encoderButtonPushed,
  //               /*double*/ encoderButtonDoubleClicked,
  //               /*deadman*/ onDeadmanChanged,
  //               /*min*/ -ENCODER_NUM_STEPS_BRAKE,
  //               /*max*/ ENCODER_NUM_STEPS_ACCEL);
  // throttle.setMap(GENTLE);
  // _deadmanButton.setReleasedHandler(deadmanReleased);
}
