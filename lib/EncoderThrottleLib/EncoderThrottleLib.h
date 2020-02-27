#ifndef ENCODERTHROTTLELIB_H

#ifndef Arduino
#include <Arduino.h>
#endif

#ifndef Wire
#include <Wire.h>
#endif
#include <i2cEncoderLibV2.h>

i2cEncoderLibV2 Encoder(0x01); /* A0 is soldered */

enum ThrottleMap
{
  LINEAR,
  GENTLE,
};

struct MapEncoderToThrottle
{
  int value;
  uint8_t throttle;
};

MapEncoderToThrottle gentle_map[] = {
    {-8, 0},
    {-7, 18},
    {-6, 36},
    {-5, 53},
    {-4, 70},
    {-3, 88},
    {-2, 106},
    {-1, 122},
    {0, 127},
    {1, 133},
    {2, 149},
    {3, 167},
    {4, 185},
    {5, 202},
    {6, 219},
    {7, 237},
    {8, 255},
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
      EncoderThrottleCb encoderButtonDoubleClickCb,
      EncoderThrottleCb encoderDeadmanChanged,
      int8_t min,
      int8_t max)
  {
    _encoderChangedCb = encoderChangedCb;
    _encoderButtonPushedCb = encoderButtonPushedCb;
    _encoderButtonDoubleClickCb = encoderButtonDoubleClickCb;
    _encoderDeadmanChanged = encoderDeadmanChanged;
    _min = min;
    _max = max;
    _mapped_max = 255;
    _mapped_min = 0;
    _deadmanHeld = false;

    _useMap = ThrottleMap::LINEAR;

    Encoder.reset();
    Encoder.begin(i2cEncoderLibV2::INT_DATA |
                  i2cEncoderLibV2::WRAP_DISABLE |
                  i2cEncoderLibV2::DIRE_RIGHT |
                  i2cEncoderLibV2::IPUP_ENABLE |
                  i2cEncoderLibV2::RMOD_X1 |
                  i2cEncoderLibV2::STD_ENCODER);

    Encoder.writeGP2conf(i2cEncoderLibV2::GP_IN |
                         i2cEncoderLibV2::GP_PULL_EN |
                         i2cEncoderLibV2::GP_INT_DI);

    Encoder.onIncrement = _encoderChangedCb;
    Encoder.onDecrement = _encoderChangedCb;
    Encoder.onButtonPush = _encoderButtonPushedCb;
    Encoder.onButtonDoublePush = _encoderButtonDoubleClickCb;
    Encoder.onGP2Rise = _encoderDeadmanChanged;
    Encoder.onGP2Fall = _encoderDeadmanChanged;

    Encoder.writeCounter((int32_t)0);    /* Reset the counter value */
    Encoder.writeMax((int32_t)max);      /* Set the maximum threshold*/
    Encoder.writeMin((int32_t)min);      /* Set the minimum threshold */
    Encoder.writeStep((int32_t)1);       /* Set the step to 1*/
    Encoder.writeAntibouncingPeriod(20); /* Set an anti-bouncing of 200ms */
    Encoder.writeDoublePushPeriod(50);   /*Set a period for the double push of 500ms */
    Encoder.updateStatus();
  }

  bool _deadmanHeld = false;

  void loop()
  {
    Encoder.updateStatus();
  }

  void clear()
  {
    Encoder.writeCounter(0);
  }

  uint8_t _getThrottleFromMap(int counter)
  {
    switch (_useMap)
    {
    case ThrottleMap::LINEAR:
      return map(counter, 0, _max, 127, _mapped_max);
    case ThrottleMap::GENTLE:
      // find item in normal map
      for (uint8_t i = 0; i < sizeof(gentle_map); i++)
      {
        if (gentle_map[i].value == counter)
        {
          return gentle_map[i].throttle;
        }
      }
      return 127;
    }
    return 127;
  }

  uint8_t mapCounterToThrottle()
  {
    int counter = Encoder.readCounterByte();
    if (counter > 0 && _deadmanHeld == false)
    {
      Encoder.writeCounter((int32_t)0);
      return 127;
    }
    return _getThrottleFromMap(counter);
  }

  void setMap(ThrottleMap mapNum)
  {
    _useMap = mapNum;
  }

  ThrottleMap getMap()
  {
    return _useMap;
  }

private:
  void _onDeadmanChanged(i2cEncoderLibV2 *obj)
  {
    _deadmanHeld = Encoder.readGP2() == 0;
    _encoderChangedCb(&Encoder);
#ifdef PRINT_THROTTLE
    DEBUGVAL(_deadmanHeld);
#endif
  }

  EncoderThrottleCb _encoderChangedCb;
  EncoderThrottleCb _encoderButtonPushedCb;
  EncoderThrottleCb _encoderButtonDoubleClickCb;
  EncoderThrottleCb _encoderDeadmanChanged;

  int _min, _max;
  int _mapped_min, _mapped_max;
  ThrottleMap _useMap;
};

#endif