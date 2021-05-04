#ifndef Arduino
#include <Arduino.h>
#endif

#include <AS5600.h>
#include <FastMap.h>

class MagneticEncoderThrottle
{
public:
  void init(AMS_5600 *ams, uint8_t sweep_angle)
  {
    _ams = ams;
    _sweep_angle = sweep_angle;

    _throttle_map.init(0, sweep_angle * 2, 0, 255);

    centre(/*print*/ true);
  }

  uint8_t get(bool enabled = true)
  {
    _current = _convertRawAngleToDegrees(_ams->getRawAngle());

    float delta = _last - _current;
    float angle = 0;

    if (_wraps)
    {
      if (_current > _min + 360)
        angle = _lower_map.map(_current);
      else
        angle = _upper_map.constrainedMap(_current);
    }
    else
    {
      // if (_current > _max && _state == )
      angle = _upper_map.constrainedMap(_current);
    }

    uint8_t throttle = _throttle_map.map(angle);

    if (abs(delta) > 0.5)
    {
      Serial.printf("raw: %0.1fdeg   ", _current);
      Serial.printf("mapped to: %0.1fdeg   ", angle);
      Serial.printf("throttle: %d   ", throttle);
      Serial.printf("state: %s  ", _getState(_state));
      Serial.printf("\n");
    }
    _last = _current;

    return throttle;
  }

  void centre(bool print = false)
  {
    _current = _convertRawAngleToDegrees(_ams->getRawAngle());
    _centre = _current;
    _last = _current;
    _max = _centre + _sweep_angle;
    if (_max > 360)
      _max = _max - 360;
    _min = _centre - _sweep_angle;
    _wraps = _centre < _sweep_angle || _centre > 360 - _sweep_angle;
    _state = ACCELERATING;

    _setMaps(print);
  }

  const char *_getState(ThrottleState state)
  {
    switch (state)
    {
    case LIMIT_LOW:
      return "LIMIT_LOW";
    case BRAKING:
      return "BRAKING";
    case ACCELERATING:
      return "ACCELERATING";
    case LIMIT_HIGH:
      return "LIMIT_HIGH";
    }
    return "OUT OF RANGE (_getState())";
  }

private:
  AMS_5600 *_ams = nullptr;

  float _convertRawAngleToDegrees(word newAngle)
  {
    /* Raw data reports 0 - 4095 segments, which is 0.087 of a degree */
    float retVal = newAngle * 0.087;
    return retVal;
  }

  void _setMaps(bool print = false)
  {
    if (_wraps)
    {
      _lower[MIN_IN] = _min + 360.0;
      _lower[MAX_IN] = 360.0;
      _lower[MIN_OUT] = 0;
      _lower[MAX_OUT] = abs(_min);
      _lower_map.init(_lower[MIN_IN], _lower[MAX_IN], _lower[MIN_OUT], _lower[MAX_OUT]);

      _upper[MIN_IN] = 0;
      _upper[MAX_IN] = _max;
      _upper[MIN_OUT] = abs(_min);
      _upper[MAX_OUT] = _sweep_angle * 2.0;
      _upper_map.init(_upper[MIN_IN], _upper[MAX_IN], _upper[MIN_OUT], _upper[MAX_OUT]);
    }
    else
    {
      _upper_map.init(_min, _max, 0, _sweep_angle * 2.0);
    }

    if (print)
      _print();
  }

  void _print()
  {
    if (_wraps)
    {
      Serial.printf("WRAPS | ");
      Serial.printf("current: %.1f ", _current);
      Serial.printf("lower: [%.1f - %.1f] => [%.1f - %.1f] |  ", _lower[MIN_IN], _lower[MAX_IN], _lower[MIN_OUT], _lower[MAX_OUT]);
      Serial.printf("upper: [%.1f - %.1f] => [%.1f - %.1f]", _upper[MIN_IN], _upper[MAX_IN], _upper[MIN_OUT], _upper[MAX_OUT]);
      Serial.println();
    }
    else
    {
      Serial.printf("NORMAL | ");
      Serial.printf("current: %.1f | ", _current);
      Serial.printf("upper: [%.1f - %.1f] maps to [%.1f - %.1f] ", _min, _max, 0, _sweep_angle * 2.0);
      Serial.println();
    }
  }

  FastMap _lower_map, _upper_map, _throttle_map;

  bool _wraps = false;
  uint8_t _sweep_angle = 0;
  float _last = -1,
        _current = 0,
        _centre = 0,
        _max = 0,
        _min = 0;

  enum ThrottleState
  {
    LIMIT_LOW = 0,
    BRAKING,
    ACCELERATING,
    LIMIT_HIGH,
  };

  ThrottleState _state;

  enum MapConsts
  {
    MIN_IN = 0,
    MAX_IN,
    MIN_OUT,
    MAX_OUT,
  };

  float _lower[4];
  float _upper[4];
};