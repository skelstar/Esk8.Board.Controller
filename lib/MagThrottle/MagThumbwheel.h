#pragma once

#define MAGNETIC_THROTTLE_H

#ifndef Arduino
#include <Arduino.h>
#endif

#include <Wire.h>
#include <AS5600.h>
#include <FastMap.h>
#include <Fsm.h>
#include <QueueManager.h>

// https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf
AMS_5600 ams5600;

//=========================================

class MagneticThrottleBase
{

public:
  typedef bool (*GetBoolean_Cb)();

  bool printThrottle = false,
       printDebug = false;

protected:
  uint8_t _accel_direction = DIR_CLOCKWISE;
  uint8_t _throttle = 127;
  GetBoolean_Cb _throttleEnabled_cb = nullptr;
  SemaphoreHandle_t _i2c_mux = nullptr;

public:
  void setAccelDirection(uint8_t accelDirection)
  {
    _accel_direction = accelDirection == DIR_CLOCKWISE || accelDirection == DIR_ANIT_CLOCKWISE
                           ? accelDirection
                           : DIR_CLOCKWISE;
    Serial.printf("MagneticThumbwheel: DIR=%s\n", _accel_direction == DIR_CLOCKWISE ? "CW" : "CCW");
  }

  uint8_t get()
  {
    return _throttle;
  }
};

//=========================================

class MagneticThumbwheelClass : public MagneticThrottleBase
{

public:
private:
  float _centre = 0.0,
        _prev_deg = 0.0,
        _sweep = 0.0,
        _deadzone = 5.0,
        _max_delta_limit = 0.0,
        _min_delta_limit = 0.0;
  FastMap _accelmapper, _brakemapper;

public:
  MagneticThumbwheelClass()
  {
  }

  void init(SemaphoreHandle_t i2c_mux)
  {
    _i2c_mux = i2c_mux;
    assert(_i2c_mux != nullptr);
    assert(_throttleEnabled_cb != nullptr);
    centre();
  }

  void setSweepAngle(float sweep)
  {
    _sweep = sweep;
    Serial.printf("MagneticThumbwheel: SWEEP=%.1f\n", _sweep);
  }

  void setDeadzone(float degrees)
  {
    assert(degrees > 0.0);
    _deadzone = degrees;
  }

  //----------------------------------------
  void update()
  {
    bool changed = false;

    float deg = _centre; // default
    if (take(_i2c_mux, TICKS_100ms))
    {
      deg = _convertRawAngleToDegrees(ams5600.getRawAngle());
      give(_i2c_mux);
    }
    float delta = _getDeltaDegrees(deg, _centre);
    bool acrossZeroDegrees = abs(delta) > 180.0;

    if (acrossZeroDegrees)
      delta = _deltaAcrossZeroDegress(deg, _prev_deg);

    uint8_t rawThrottle = 127;
    float t = 127.0;

    bool usesMapper = abs(delta) >= _deadzone;

    if (usesMapper)
    {
      if (_accel_direction == DIR_CLOCKWISE)
      {
        t = delta > _deadzone
                ? _accelmapper.constrainedMap(delta)
                : _brakemapper.constrainedMap(delta);
      }
      else if (_accel_direction == DIR_ANIT_CLOCKWISE)
      {
        t = delta < -_deadzone
                ? _accelmapper.constrainedMap(delta)
                : _brakemapper.constrainedMap(delta);
      }
    }
    rawThrottle = (int)t;
    Serial.printf("%s: delta=%.1f %d %.1f dir=%s\n",
                  delta > _deadzone
                      ? "Accel"
                  : delta < -_deadzone ? "Brake"
                                       : " --- ",
                  delta,
                  rawThrottle,
                  t,
                  _accel_direction == DIR_CLOCKWISE ? "CW" : "CCW");

    if (rawThrottle > 127 && _throttleEnabled_cb() == false)
      _throttle = 127;

    _throttle = rawThrottle;

    changed = true;
  }

  void
  centre()
  {
    if (take(_i2c_mux, TICKS_50ms))
    {
      _centre = _convertRawAngleToDegrees(ams5600.getRawAngle());
      give(_i2c_mux);
    }

    _calibrateAccelBrakeMaps();

    _throttle = 127;
    _prev_deg = _centre;
    update();
  }

  void setThrottleEnabledCb(GetBoolean_Cb cb)
  {
    _throttleEnabled_cb = cb;
  }

  bool connect()
  {
    if (take(_i2c_mux, TICKS_50ms))
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
      give(_i2c_mux);
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

  void _calibrateAccelBrakeMaps()
  {
    if (_accel_direction == DIR_CLOCKWISE)
    {
      _accelmapper.init(_deadzone, _sweep, 127.0, 255.0);
      _brakemapper.init(0 - _sweep, 0 - _deadzone, 0.0, 127.0);
    }
    else
    {
      _accelmapper.init(-_deadzone, -_sweep, 127, 0);
      _brakemapper.init(_deadzone, _sweep, 127, 255);
    }
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
