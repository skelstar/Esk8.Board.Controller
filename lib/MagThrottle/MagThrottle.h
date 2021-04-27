#pragma once

#define MAGNETIC_THROTTLE_H

#ifndef Arduino
#include <Arduino.h>
#endif

#include <Wire.h>
#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>

// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf
AMS_5600 ams5600;

class MagneticThrottleClass
{
  typedef bool (*GetBoolean_Cb)();

  enum ThrottleShape
  {
    NONE = 0,
    ABSOLUTE,
    RUNNING,
  };

public:
  bool printThrottle = false,
       printDebug = false;

private:
  ThrottleShape _shape;
  uint8_t _accel_direction = DIR_CLOCKWISE;
  uint8_t _throttle = 127;
  float _raw_throttle = 0.0;
  float _centre = 0.0,
        _prev_deg = 0.0,
        _sweep = 0.0,
        _max_delta_limit = 0.0,
        _min_delta_limit = 0.0;
  GetBoolean_Cb _throttleEnabled_cb = nullptr;

public:
  MagneticThrottleClass()
  {
  }

  void init()
  {
    assert(_throttleEnabled_cb != nullptr);
    centre();
  }

  void setThrottleShape(ThrottleShape shape)
  {
    _shape = shape;
  }

  void setAccelDirection(uint8_t accelDirection)
  {
    _accel_direction = accelDirection == DIR_CLOCKWISE || accelDirection == DIR_ANIT_CLOCKWISE
                           ? accelDirection
                           : DIR_CLOCKWISE;

    Serial.printf("MagThrottle: DIR=%s\n", _accel_direction == DIR_CLOCKWISE ? "CW" : "CCW");
  }

  void setSweepAngle(float sweep)
  {
    _sweep = sweep;
    Serial.printf("MagThrottle: SWEEP=%.1f\n", _sweep);
  }

  void setDeltaLimits(float min, float max)
  {
    if (min < 0.0 || max < 0.0)
    {
      Serial.printf("ERROR: magnetic throttle min or max delta must be > 0.0\n");
      return;
    }
    _min_delta_limit = min;
    _max_delta_limit = max;
    Serial.printf("Magnetic throttle min=%.1fdeg max=%.1fdeg\n", min, max);
  }

  uint8_t get()
  {
    return _throttle;
  }

  //----------------------------------------
  void update(bool force_print = false)
  {
    bool changed = false;

    float deg = _centre; // default
    if (take(mux_I2C, TICKS_100ms))
    {
      deg = _convertRawAngleToDegrees(ams5600.getRawAngle());
      give(mux_I2C);
    }
    float delta = _getDeltaDegrees(deg, _prev_deg);
    bool acrossZeroDegrees = abs(delta) > 180.0;

    if (acrossZeroDegrees)
      delta = _deltaAcrossZeroDegress(deg, _prev_deg);

    // now we have the amount the throttle has moved

    // make sure not too radical
    // make sure is more than minimum (to eliminate drift/noise)

    bool tooMuch = _max_delta_limit > 0.0 && abs(delta) > _max_delta_limit;
    bool tooLittle = abs(delta) < _min_delta_limit;

    if (!tooLittle)
    {
      if (tooMuch)
        delta = _limitDeltaToMax(delta);

      _throttle = _constrainThrottle(_throttle, delta);

      if (_throttle > 127 && _throttleEnabled_cb() == false)
        _throttle = 127;

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
        Serial.printf("%s ", acrossZeroDegrees ? "EDGE" : "----");
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

    if (changed)
      // we don't want to update if was below min_delta
      _prev_deg = deg;
    if (printThrottle)
      Serial.printf("prev_deg=%.1f deg=%.1f\n", _prev_deg, deg);
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

private:
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

  float _getDeltaDegrees(float deg, float last)
  {
    return _accel_direction == DIR_CLOCKWISE
               ? deg - last
               : last - deg;
  }

  float _deltaAcrossZeroDegress(float deg, float prev)
  {
    deg = prev > deg
              ? deg + 360.0
              : deg - 360.0;
    return _getDeltaDegrees(deg, prev);
  }

  uint8_t _constrainThrottle(uint8_t p_throttle, float delta)
  {
    int16_t running = p_throttle + (delta / 360.0) * 255;
    return constrain(running, 0, 255);
  }

  float _limitDeltaToMax(float delta)
  {
    float oldDelta = delta;
    delta = delta > 0 ? _max_delta_limit : 0 - _max_delta_limit;
    Serial.printf("oldDelta %.1f limited to %.1f\n", oldDelta, delta);
    return delta;
  }
};
