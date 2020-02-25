#ifndef ENCODERTHROTTLELIB_H

#ifndef Arduino
#include <Arduino.h>
#endif

#ifndef Wire
#include <Wire.h>
#endif
#include <i2cEncoderLibV2.h>

i2cEncoderLibV2 Encoder(0x01); /* A0 is soldered */

enum ThrottleMode
{
  BEGINNER,
  ADVANCED,
};

class EncoderThrottleLib
{
  typedef void (*EncoderThrottleCb)(i2cEncoderLibV2 *);

public:
  EncoderThrottleLib()
  {
  }

  void init(
      EncoderThrottleCb encoderChangedCb,
      EncoderThrottleCb encoderButtonPushedCb,
      int8_t min,
      int8_t max)
  {
    _encoderChangedCb = encoderChangedCb;
    _encoderButtonPushedCb = encoderButtonPushedCb;
    _min = min;
    _max = max;
    mode = ADVANCED;
    _mapped_max = 255;
    _mapped_min = 0;

    Encoder.reset();
    Encoder.begin(
        i2cEncoderLibV2::INT_DATA |
        i2cEncoderLibV2::WRAP_DISABLE |
        i2cEncoderLibV2::DIRE_RIGHT |
        i2cEncoderLibV2::IPUP_ENABLE |
        i2cEncoderLibV2::RMOD_X1 |
        i2cEncoderLibV2::STD_ENCODER);
    Encoder.onIncrement = _encoderChangedCb;
    Encoder.onDecrement = _encoderChangedCb;
    Encoder.onButtonPush = _encoderButtonPushedCb;
    Encoder.writeCounter((int32_t)0);    /* Reset the counter value */
    Encoder.writeMax((int32_t)max);      /* Set the maximum threshold*/
    Encoder.writeMin((int32_t)min);      /* Set the minimum threshold */
    Encoder.writeStep((int32_t)1);       /* Set the step to 1*/
    Encoder.writeAntibouncingPeriod(20); /* Set an anti-bouncing of 200ms */
    Encoder.writeDoublePushPeriod(50);   /*Set a period for the double push of 500ms */
  }

  void loop()
  {
    Encoder.updateStatus();
  }

  uint8_t mapCounterToThrottle(bool deadmanPressed)
  {
    int counter = Encoder.readCounterByte();
    if (counter > 0)
    {
      if (deadmanPressed == false)
      {
        Encoder.writeCounter((int32_t)0);
        return 127;
      }
      return map(counter, 0, _max, 127, _mapped_max);
    }
    else if (counter < 0)
    {
      return map(counter, _min, 0, _mapped_min, 127);
    }
    return 127;
  }

  void setMode(ThrottleMode mode)
  {
    _mode = mode;
    switch (mode)
    {
    case ADVANCED:
      _mapped_max = 255;
      _mapped_min = 0;
      break;
    case BEGINNER:
      uint8_t limit = 127 * 0.5;
      _mapped_max = 127 + limit;
    }
  }

private:
  EncoderThrottleCb _encoderChangedCb;
  EncoderThrottleCb _encoderButtonPushedCb;
  ThrottleMode _mode;
  int _min, _max;
  int _mapped_min, _mapped_max;
};

#endif