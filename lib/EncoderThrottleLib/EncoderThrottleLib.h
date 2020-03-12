#ifndef ENCODERTHROTTLELIB_H

#ifndef Arduino
#include <Arduino.h>
#endif

#ifndef Wire
#include <Wire.h>
#endif
#include <i2cEncoderLibV2.h>

#ifndef Smoothed
#include <Smoothed.h>
#endif

// https : //github.com/skelstar/Esk8.Board.Controller/blob/670e83203e9f9d6687bc75beccb61d83b0c71708/include/throttle.h

i2cEncoderLibV2 Encoder(0x01); /* A0 is soldered */

Smoothed<float> smoothedThrottle;

enum ThrottleMap
{
  LINEAR,
  GENTLE,
  SMOOTHED,
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

MapEncoderToThrottle calc_map[30];

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
      int32_t min,
      int32_t max)
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

    uint8_t smoothedBufferLength = 1 * (1000 / READ_TRIGGER_PERIOD);
    DEBUGVAL(smoothedBufferLength);
    smoothedThrottle.begin(SMOOTHED_AVERAGE, smoothedBufferLength);

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

    // Encoder.onIncrement = _encoderChangedCb;
    // Encoder.onDecrement = _encoderChangedCb;
    Encoder.onButtonPush = _encoderButtonPushedCb;
    Encoder.onButtonDoublePush = _encoderButtonDoubleClickCb;
    // Encoder.onGP2Rise = _encoderDeadmanChanged;
    // Encoder.onGP2Fall = _encoderDeadmanChanged;

    Encoder.writeCounter((int32_t)0);    /* Reset the counter value */
    Encoder.writeMax(_max);              /* Set the maximum threshold*/
    Encoder.writeMin(_min);              /* Set the minimum threshold */
    Encoder.writeStep((int32_t)1);       /* Set the step to 1*/
    Encoder.writeAntibouncingPeriod(20); /* Set an anti-bouncing of 200ms */
    Encoder.writeDoublePushPeriod(50);   /*Set a period for the double push of 500ms */
    Encoder.updateStatus();
  }

  bool _deadmanHeld = true;

  void loop(bool deadmanHeld)
  {
    manageDeadmanChange(deadmanHeld);

    Encoder.updateStatus();

    uint8_t t = mapCounterToThrottle();

    if (_useMap == ThrottleMap::SMOOTHED)
    {
      smoothedThrottle.add(t);

      // uint8_t smoothedt = smoothedThrottle.get();
      // if (currentThrottle != smoothedt)
      // {
      //   controller_packet.throttle = smoothedt;
      //   DEBUGVAL(controller_packet.throttle);
      // }
      // uint8_t smoothed_throttle = _getThrottleFromMap(Encoder.readCounterByte());
    }
    else
    {
      if (_oldThrottle != t)
      {
        DEBUGVAL(t);
      }
    }
    _oldThrottle = t;
  }

  void clear()
  {
    Encoder.writeCounter(0);
  }

  void manageDeadmanChange(bool deadmanHeld)
  {
    if (deadmanHeld != _deadmanHeld)
    {
      _deadmanHeld = deadmanHeld;
      if (!_deadmanHeld)
      {
        Encoder.writeCounter((int32_t)0);
        Encoder.writeMax((int32_t)0);
        DEBUGVAL(_deadmanHeld);
      }
      else
      {
        Encoder.writeMax(_max);
        DEBUGVAL(_deadmanHeld);
      }
    }
  }

  uint8_t _getThrottleFromMap(int counter)
  {
    switch (_useMap)
    {
    case ThrottleMap::LINEAR:
      if (counter >= 0)
      {
        return map(counter, 0, _max, 127, 255);
      }
      else
      {
        return map(counter, _min, 0, 0, 127);
      }
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
    case ThrottleMap::SMOOTHED:
      uint8_t m = counter >= 0
                      ? map(counter, 0, _max, 127, 255)
                      : map(counter, _min, _max, 0, 127);
      smoothedThrottle.add(m);
      return smoothedThrottle.get();
    }
    return 127;
  }

  uint8_t mapCounterToThrottle(bool print = false)
  {
    int counter = Encoder.readCounterByte();
    uint8_t throttle = _getThrottleFromMap(counter);
    return throttle;
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
  EncoderThrottleCb _encoderChangedCb;
  EncoderThrottleCb _encoderButtonPushedCb;
  EncoderThrottleCb _encoderButtonDoubleClickCb;
  EncoderThrottleCb _encoderDeadmanChanged;

  int32_t _min, _max;
  int _mapped_min, _mapped_max;
  ThrottleMap _useMap;
  uint8_t _oldThrottle;
};

#endif