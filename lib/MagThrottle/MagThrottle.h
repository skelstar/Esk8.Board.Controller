#ifndef Arduino
#include <Arduino.h>
#endif

#include <AS5600.h>
#include <FastMap.h>

class MagThrottle
{
public:
  void init(AMS_5600 *ams, uint8_t sweep_angle)
  {
    _ams = ams;
    _sweep_angle = sweep_angle;

    _throttle_map.init(0, sweep_angle * 2, 0, 255);

    centre();
  }

  uint8_t get(bool enabled = true)
  {
    _current = _convertRawAngleToDegrees(_ams->getRawAngle());

    float delta = _last - _current;
    float angle = 0;

    if (_wraps)
    {
      if (_current > _min + 360)
        angle = _lower_map.constrainedMap(_current);
      else
        angle = _upper_map.constrainedMap(_current);
    }
    else
    {
      angle = _upper_map.constrainedMap(_current);
    }

    uint8_t throttle = _throttle_map.map(angle);

    if (abs(delta) > 0.5)
    {
      Serial.printf("raw: %0.1fdeg   ", _current);
      Serial.printf("mapped to: %0.1fdeg   ", angle);
      Serial.printf("throttle: %d   ", throttle);
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

    _setMaps(print);
  }

private:
  AMS_5600 *_ams = nullptr;

  float _convertRawAngleToDegrees(word newAngle)
  {
    /* Raw data reports 0 - 4095 segments, which is 0.087 of a degree */
    float retVal = newAngle * 0.087;
    ang = retVal;
    return retVal;
  }

  void _setMaps(bool print = false)
  {
    if (_wraps)
    {
      l_min_i = _min + 360.0;
      l_max_i = 360.0;
      l_min_o = 0;
      l_max_o = abs(_min);
      _lower_map.init(l_min_i, l_max_i, l_min_o, l_max_o);

      u_min_i = 0;
      u_max_i = _max;
      u_min_o = abs(_min);
      u_max_o = _sweep_angle * 2;
      _upper_map.init(u_min_i, u_max_i, u_min_o, u_max_o);
    }
    else
    {
      _upper_map.init(_min, _max, 0, _sweep_angle * 2);
    }
    if (print)
      _print();
  }

  void _print()
  {
    if (outlier)
    {
      Serial.printf("outlier ");
      Serial.printf("current: %.1f ", current);
      Serial.printf("lower: %.1f->%.1f => %.1f->%.1f |  ", l_min_i, l_max_i, l_min_o, l_max_o);
      Serial.printf("upper: %.1f->%.1f => %.1f->%.1f", u_min_i, u_max_i, u_min_o, u_max_o);
      Serial.println();
    }
    else
    {
      Serial.printf("normal current: %.1f upper: %.1f->%.1f \n", current, mapped_min, mapped_max);
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
  float l_min_i, l_min_o, l_max_i, l_max_o;
  float u_min_i, u_min_o, u_max_i, u_max_o;
};