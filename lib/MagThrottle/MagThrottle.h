#ifndef Arduino
#include <Arduino.h>
#endif

#include <Wire.h>
#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>

#include <SparkFun_Qwiic_Button.h>
//https://www.sparkfun.com/products/15932
// int _buttoni2cAddr = 0x6f;
QwiicButton button;

namespace MagThrottle
{
  // https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

  enum Direction
  {
    CLOCKWISE = 0,
    ANTI_CLOCKWISE
  };

  namespace // private
  {
    Direction _direction = CLOCKWISE;
    int16_t _throttle = 127;
    uint8_t _bar_len = 10;
    float _centre = 0.0,
          _last = 0.0,
          _sweep = 0.0,
          _limit = 0.0;
    FastMap _bar_map;

    uint8_t _getPowerString(int idx, float delta, char *buff)
    {
      buff[idx++] = delta > 0.0 ? '#' : ' ';
      buff[idx++] = delta > 10.0 ? '#' : ' ';
      buff[idx++] = delta > 20.0 ? '#' : ' ';
      buff[idx++] = delta > 30.0 ? '#' : ' ';
      buff[idx++] = delta > 40.0 ? '#' : ' ';
      buff[idx++] = delta > 50.0 ? '!' : ' ';

      return idx;
    }

    uint8_t _getThrottleBar(uint8_t idx, char *buff, uint8_t throttle)
    {
      uint8_t m = _bar_map.constrainedMap(throttle);
      // bottom
      buff[idx++] = '[';
      for (int j = 0; j < _bar_len; j++)
      {
        buff[idx] = j < m ? ' ' : '<';
        idx++;
      }
      buff[idx++] = '+';
      // upper
      for (int j = _bar_len + 1; j <= _bar_len + 1 + _bar_len; j++)
      {
        buff[idx] = j > m ? ' ' : '>';
        idx++;
      }
      buff[idx++] = ']';

      return idx;
    }

    void _throttleString(float delta, uint16_t throttle, char *buff)
    {
      int i = 0;

      i = _getPowerString(i, delta, buff);
      i = _getThrottleBar(i, buff, throttle);

      buff[i] = '\0';
    }

    float _convertRawAngleToDegrees(word angle)
    {
      return angle * 0.087;
    }
  }

  void update(bool enabled, bool force_print = false)
  {
    float deg = _convertRawAngleToDegrees(ams5600.getRawAngle());
    float adj = deg;
    float delta = _last - deg;
    bool edge = false;

    if (abs(delta) > 180.0)
    {
      edge = true;
      if (_last > deg)
        adj += 360.0;
      else
        adj -= 360.0;
      delta = _last - adj;
      if (_direction == ANTI_CLOCKWISE)
        delta = adj - _last;
    }

    if (abs(delta) < _limit)
    {
      _throttle += (delta / 360.0) * 255;

      if (_throttle > 255)
        _throttle = 255;
      else if (_throttle < 0)
        _throttle = 0;

      if (_throttle > 127 && enabled == false)
        _throttle = 127;

      if (abs(delta) > 0.5 || force_print)
      {
        char b[50];
        _throttleString(abs(delta), _throttle, b);
        Serial.printf("%s ", edge ? "EDGE" : "----");
        Serial.printf("| delta: %05.1f", delta);
        Serial.printf("| throttle: %03d", _throttle);
        Serial.printf("  %s \n", b);
      }
    }
    else
    {
      Serial.printf("%s ", edge ? "EDGE" : "----");
      Serial.printf("| limit (%.1f) EXCEEDED!!! ", delta);
      Serial.printf("| throttle: %03d", _throttle);
      Serial.printf("  \n");
    }

    _last = deg;
  }

  void centre()
  {
    _centre = _convertRawAngleToDegrees(ams5600.getRawAngle());
    _throttle = 127;
    _last = _centre;
    update(/*enabled*/ true, /*force*/ true);
  }

  void init(float sweep, float limit, Direction direction)
  {
    _sweep = sweep;
    _limit = limit;
    _direction = direction;
    _bar_map.init(0, 255, 0, _bar_len + 1 + _bar_len);
    centre();
  }
}