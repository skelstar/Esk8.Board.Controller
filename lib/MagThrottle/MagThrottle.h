#pragma once

#define MAGNETIC_THROTTLE_H

#ifndef Arduino
#include <Arduino.h>
#endif

#include <Wire.h>
#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>

namespace MagneticThrottle
{
  // variables
  bool printThrottle = false,
       printDebug = false;

  // https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf
  AMS_5600 ams5600;

  typedef bool (*GetBoolean_Cb)();

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
    GetBoolean_Cb _throttleEnabled_cb = nullptr;

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

    float limitDeltaToMax(float delta, uint8_t throttle)
    {
      float oldDelta = delta;
      delta = delta > 0 ? _max_delta_limit : 0 - _max_delta_limit;
      Serial.printf("oldDelta %.1f limited to %.1f\n", oldDelta, delta);
      return delta;
    }

  }

  uint8_t get()
  {
    return _throttle;
  }

  void update(bool force_print = false)
  {
    bool changed = false;

    float deg = _centre;
    // TODO is this the bext way to handle this?
    if (take(mux_I2C, TICKS_100ms))
    {
      deg = _convertRawAngleToDegrees(ams5600.getRawAngle());
      give(mux_I2C);
    }
    float adj = deg;
    float delta = getDelta(deg, _prev_deg);
    bool transitionsAcrossZero = abs(delta) > 180.0;

    if (transitionsAcrossZero)
    {
      if (_prev_deg > deg)
        adj += 360.0;
      else
        adj -= 360.0;
      delta = getDelta(adj, _prev_deg);
    }

    // are we allowed to be accelerating?
    if (_throttle > 127 && !_throttleEnabled_cb())
    {
      _throttle = 127;
      changed = true;
    }

    // make sure not too radical
    // make sure is more than minimum (to eliminate drift/noise)

    bool tooMuch = abs(delta) > _max_delta_limit;
    bool tooLittle = abs(delta) < _min_delta_limit;

    if (!tooLittle)
    {
      if (tooMuch)
        delta = limitDeltaToMax(delta, _throttle);

      int16_t running = _throttle + (delta / 360.0) * 255;

      _throttle = constrain(running, 0, 255);

      if (_throttle > 127 && _throttleEnabled_cb() == false)
      {
        _throttle = 127;
      }

      if (printThrottle || force_print)
      {
        char b[50];
        _throttleString(abs(delta), _throttle, b);
      }
      changed = true;
    }

    if (printThrottle || force_print)
    {
      if (changed || force_print)
      {
        char b[50];
        _throttleString(abs(delta), _throttle, b);

        Serial.printf("thr: %03d ", _throttle);
        Serial.printf("%s ", transitionsAcrossZero ? "EDGE" : "----");
        Serial.printf("| delta: %05.1f", delta);
        Serial.printf("| delta_limit (%.1f-%.1f) %s ",
                      _min_delta_limit, _max_delta_limit,
                      tooLittle ? "UNDER"
                      : tooMuch ? "LIMIT"
                                : "-----");
        Serial.printf("  %s", b); // throttleString
        Serial.printf("\n");
      }
    }
    _prev_deg = deg;
  }

  void centre()
  {
    if (take(mux_I2C, TICKS_50ms))
    {
      _centre = _convertRawAngleToDegrees(ams5600.getRawAngle());
      give(mux_I2C);
    }
    _raw_throttle = _centre;
    _throttle = 127;
    _prev_deg = _centre;
    update(/*force*/ true);
  }

  void setThrottleEnabledCb(GetBoolean_Cb cb)
  {
    _throttleEnabled_cb = cb;
  }

  /*
  - sweep: the full range of sweep in degrees
  - max_delta_limit: the most num degrees in one sample period
  - min_delta_limit: the min num degrees in one sample period (to prevent drift)
  - direction: DIR_CLOCKWISE or DIR_ANIT_CLOCKWISE
  */
  void init(float sweep, float max_delta_limit, float min_delta_limit, uint8_t direction)
  {
    if (_throttleEnabled_cb == nullptr)
    {
      Serial.printf("ERROR: throttleEnabledCallback is not set!!!\n");
      return;
    }

    Serial.printf("MagThrottle: SWEEP=%.1f MIN=%.1f MAX=%.1f DIR=%s\n",
                  sweep, min_delta_limit, max_delta_limit, direction == DIR_CLOCKWISE ? "CW" : "CCW");
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
    if (take(mux_I2C, TICKS_50ms))
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
      give(mux_I2C);
    }
    return true;
  }
}