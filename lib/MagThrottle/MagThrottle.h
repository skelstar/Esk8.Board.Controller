#pragma once

#ifndef Arduino
#include <Arduino.h>
#endif

#include <Wire.h>
#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>

namespace MagneticThrottle
{
  // https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

  typedef bool (*ThrottleEnabled_Cb)();

  namespace // private
  {
    uint8_t _accel_direction = DIR_CLOCKWISE;
    uint8_t _throttle = 127;
    float _raw_throttle = 0.0;
    float _centre = 0.0,
          _prev_deg = 0.0,
          _sweep = 0.0,
          _max_delta_limit = 0.0,
          _min_delta_limit = 0.0;
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

    void _throttleString(float delta, uint16_t throttle, char *buff)
    {
      int i = 0;

      i = _getPowerString(i, delta, buff);

      buff[i] = '\0';
    }

    float _convertRawAngleToDegrees(word angle)
    {
      return angle * 0.087;
    }

    float getDelta(float deg, float last)
    {
      return _accel_direction == DIR_CLOCKWISE
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
    float deg = _centre;
    // TODO is this the bext way to handle this?
    if (mutex_I2C.take("MagneticThrottle: update", TICKS_50ms))
    {
      deg = _convertRawAngleToDegrees(ams5600.getRawAngle());
      mutex_I2C.give("MagneticThrottle: update"); // 1ms
    }
    float adj = deg;
    float delta = getDelta(deg, _prev_deg);
    bool transition = abs(delta) > 180.0;

    if (transition)
    {
      if (_prev_deg > deg)
        adj += 360.0;
      else
        adj -= 360.0;
      delta = getDelta(adj, _prev_deg);
    }

    if (_throttle > 127 && !_throttleEnabled_cb())
    {
      _throttle = 127;
    }

    // make sure not too radical
    // make sure is more than minimum (to eliminate drift/noise)
    if (_min_delta_limit <= abs(delta) && abs(delta) <= _max_delta_limit)
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
      {
        _throttle = 127;
      }

      if (PRINT_THROTTLE || force_print)
      {
        char b[50];
        _throttleString(abs(delta), _throttle, b);
        Serial.printf("%s ", transition ? "EDGE" : "----");
        Serial.printf("| delta: %05.1f", delta);
        Serial.printf("| throttle: %03d", _throttle);
        Serial.printf("  %s \n", b);
      }
    }
    else if (PRINT_THROTTLE || force_print)
    {
      Serial.printf("%s ", transition ? "EDGE" : "----");
      Serial.printf("| delta_limit (%.1f) EXCEEDED!!! ", delta);
      Serial.printf("| throttle: %03d", _throttle);
      Serial.printf("  \n");
    }
    _prev_deg = deg;
  }

  void centre()
  {
    if (mutex_I2C.take(__func__, TICKS_50ms))
    {
      _centre = _convertRawAngleToDegrees(ams5600.getRawAngle());
      mutex_I2C.give(__func__);
    }
    _raw_throttle = _centre;
    _throttle = 127;
    _prev_deg = _centre;
    update(/*force*/ true);
  }

  void setThrottleEnabledCb(ThrottleEnabled_Cb cb)
  {
    _throttleEnabled_cb = cb;
  }

  void init(float sweep, float max_delta_limit, float min_delta_limit, uint8_t direction)
  {
    _sweep = sweep;
    _max_delta_limit = max_delta_limit;
    _min_delta_limit = min_delta_limit;
    _accel_direction = direction == DIR_CLOCKWISE || direction == DIR_ANIT_CLOCKWISE
                           ? direction
                           : DIR_CLOCKWISE;
    centre();
  }

  bool connect()
  {
    if (mutex_I2C.take(__func__, TICKS_50ms))
    {
      if (ams5600.detectMagnet() == 0)
      {
        Serial.printf("searching....\n");
        while (1)
        {
          if (ams5600.detectMagnet() == 1)
          {
            Serial.printf("Current Magnitude: %d\n", ams5600.getMagnitude());
            break;
          }
          else
          {
            Serial.println("Can not detect magnet");
          }
          vTaskDelay(200);
        }
      }
      mutex_I2C.give(__func__);
    }
    return true;
  }
}