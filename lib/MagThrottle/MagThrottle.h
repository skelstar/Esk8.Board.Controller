#ifndef Arduino
#include <Arduino.h>
#endif

#include <Wire.h>
#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>

namespace MagThrottle
{
  // https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

  typedef bool (*ThrottleEnabled_Cb)();

  // enum Direction
  // {
  //   CLOCKWISE = 0,
  //   ANTI_CLOCKWISE
  // };

  namespace // private
  {
    uint8_t _direction = DIR_CLOCKWISE;
    uint8_t _throttle = 127;
    uint8_t _bar_len = 10;
    float _centre = 0.0,
          _last = 0.0,
          _sweep = 0.0,
          _delta_limit = 0.0;
    FastMap _bar_map;
    ThrottleEnabled_Cb _throttleEnabled_cb = nullptr;

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

    float getDelta(float deg, float last)
    {
      return _direction == DIR_CLOCKWISE
                 ? deg - last
                 : last - deg;
    }
  }

  uint8_t get()
  {
    return _throttle;
  }

  void update(bool force_print = false)
  {
    float deg = _convertRawAngleToDegrees(ams5600.getRawAngle());
    float adj = deg;
    float delta = getDelta(deg, _last);
    bool transition = abs(delta) > 180.0;

    if (transition)
    {
      if (_last > deg)
        adj += 360.0;
      else
        adj -= 360.0;
      delta = getDelta(adj, _last);

      // Serial.printf("EDGE: ");
      // Serial.printf("deg: %.1f | ", deg);
      // Serial.printf("_last: %.1f | ", _last);
      // Serial.printf("=> adj: %.1f | ", adj);
      // Serial.printf("delta: %.1f | ", delta);
      // Serial.printf("direction: %s ", _direction == DIR_CLOCKWISE ? "CLOCK" : "ANTI");
      // Serial.printf("\n");
    }

    if (abs(delta) < _delta_limit)
    {
      int16_t running = _throttle;
      running += (delta / 360.0) * 255;

      _throttle = constrain(running, 0, 255);

      if (_throttleEnabled_cb == nullptr)
      {
        Serial.printf("ERROR: throttleEnabledCallback is not set!!!\n");
        return;
      }

      if (_throttle > 127 && _throttleEnabled_cb() == false)
        _throttle = 127;

      if (PRINT_THROTTLE && (abs(delta) > 0.5 || force_print))
      {
        char b[50];
        _throttleString(abs(delta), _throttle, b);
        Serial.printf("%s ", transition ? "EDGE" : "----");
        Serial.printf("| delta: %05.1f", delta);
        Serial.printf("| throttle: %03d", _throttle);
        Serial.printf("  %s \n", b);
      }
    }
    else if (PRINT_THROTTLE)
    {
      Serial.printf("%s ", transition ? "EDGE" : "----");
      Serial.printf("| delta_limit (%.1f) EXCEEDED!!! ", delta);
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
    update(/*force*/ true);
  }

  void setThrottleEnabledCb(ThrottleEnabled_Cb cb)
  {
    _throttleEnabled_cb = cb;
  }

  void init(float sweep, float delta_limit, uint8_t direction)
  {
    _sweep = sweep;
    _delta_limit = delta_limit;
    _direction = direction == DIR_CLOCKWISE || direction == DIR_ANIT_CLOCKWISE
                     ? direction
                     : DIR_CLOCKWISE;
    _bar_map.init(0, 255, 0, _bar_len + 1 + _bar_len);
    centre();
  }
}