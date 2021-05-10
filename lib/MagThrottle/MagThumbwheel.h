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

  enum ReturnCode
  {
    OK = 0,
    MAG_NOT_DETECTED,
  };

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
    _accel_direction = accelDirection == DIR_CLOCKWISE || accelDirection == DIR_ANTI_CLOCKWISE
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

  void init(SemaphoreHandle_t i2c_mux, float sweepAngle, float deadzone, uint8_t accelDirection = DIR_CLOCKWISE)
  {
    _i2c_mux = i2c_mux;
    _sweep = sweepAngle;
    _deadzone = deadzone;
    _accel_direction = accelDirection;
    assert(_i2c_mux != nullptr);
    assert(_throttleEnabled_cb != nullptr);
    assert(_sweep > 20.0);
    assert(_deadzone > 2.0);
    assert(_accel_direction == DIR_CLOCKWISE || _accel_direction == DIR_ANTI_CLOCKWISE);
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
  ReturnCode update()
  {
    if (take(_i2c_mux, TICKS_100ms))
    {
      if (!detectMagnet())
      {
        _throttle = 127;
        give(_i2c_mux);
        return ReturnCode::MAG_NOT_DETECTED;
      }
      give(_i2c_mux);
    }
    else
    {
      Serial.printf("Couldn't take i2c mutex: MagThumbwheel.update()\n");
    }

    bool changed = false;

    float deg = _centre; // default
    if (take(_i2c_mux, TICKS_100ms))
    {
      deg = _convertRawAngleToDegrees(ams5600.getRawAngle());
      give(_i2c_mux);
    }
    float degFromCentre = _getDeltaDegrees(deg, _centre);
    bool acrossZeroDegrees = abs(degFromCentre) > 180.0;

    if (acrossZeroDegrees)
      degFromCentre = _deltaAcrossZeroDegress(deg, _prev_deg);

    uint8_t oldThrottle = _throttle;

    _throttle = 127;

    bool outsideDeadzone = abs(degFromCentre) >= _deadzone;
    if (outsideDeadzone)
    {
      if (_accel_direction == DIR_CLOCKWISE)
      {
        _throttle = degFromCentre > _deadzone
                        ? (uint8_t)_accelmapper.constrainedMap(degFromCentre)
                        : (uint8_t)_brakemapper.constrainedMap(degFromCentre);
      }
      else if (_accel_direction == DIR_ANTI_CLOCKWISE)
      {
        _throttle = degFromCentre < -_deadzone
                        ? (uint8_t)_accelmapper.constrainedMap(degFromCentre)
                        : (uint8_t)_brakemapper.constrainedMap(degFromCentre);
      }
    }

    if (_throttle > 127 && _throttleEnabled_cb() == false)
      _throttle = 127;

    changed = _throttle != oldThrottle;
    if (printThrottle && changed)
      Serial.printf("Magwheel: throttle=%d \n", _throttle);

    return ReturnCode::OK;
  }

  void centre()
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

  bool detectMagnet()
  {
    return ams5600.detectMagnet() == 1;
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
