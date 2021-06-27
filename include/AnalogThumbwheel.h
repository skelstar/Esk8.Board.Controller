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

//=========================================

typedef void (*VoidUint8Callback)(uint8_t);
typedef void (*RawThrottleCallback)(uint8_t, uint16_t, uint16_t);
typedef bool (*GetBoolean_Cb)();

class AnalogThumbwheelClass
{
public:
  bool printThrottle = false, printRawThrottle = false;

private:
  VoidUint8Callback _throttleChangedCb = nullptr;
  RawThrottleCallback _rawThrottleCallback = nullptr;
  // GetBoolean_Cb _throttleEnabled_cb = nullptr;
  FastMap _accelmapper, _brakemapper;
  int _pin = THROTTLE_PIN;
  uint8_t _oldMapped = 0, _throttle = 127;
  uint16_t _raw,
      _centre = THROTTLE_RAW_CENTRE,
      _min = THROTTLE_RAW_MIN,
      _max = THROTTLE_RAW_MAX,
      _deadband = THROTTLE_RAW_DEADBAND;

public:
  AnalogThumbwheelClass()
  {
  }

  void init(VoidUint8Callback throttleChangedCb = nullptr)
  {
    _throttleChangedCb = throttleChangedCb;
    pinMode(_pin, INPUT);
    _oldMapped = 127;
    // assert(_throttleEnabled_cb != nullptr);
  }

  uint8_t get()
  {
    return _throttle;
  }

  ThrottleStatus update(bool enabled = true, bool accelEnabled = true)
  {

    _raw = _getRaw();
    _throttle = _getMappedFromRaw(_raw);

    // print before exiting if not enabled
    if (_rawThrottleCallback)
      _rawThrottleCallback(_throttle, _raw, _centre);

    if (!enabled || (_throttle > 127 && !accelEnabled))
    {
      _throttle = 127;
      return ThrottleStatus::STATUS_OK;
    }

    if (_oldMapped != _throttle && _throttleChangedCb != nullptr)
      _throttleChangedCb(_throttle);
    _oldMapped = _throttle;

    return ThrottleStatus::STATUS_OK;
  }

  void centre()
  {
    _centre = analogRead(_pin);
    Serial.printf("centering thumbwheel: %d\n", _centre);
  }

  bool connect()
  {
    return true;
  }

  void setThrottleChanged(VoidUint8Callback cb)
  {
    _throttleChangedCb = cb;
  }

  void setRawThrottleCallback(RawThrottleCallback cb)
  {
    _rawThrottleCallback = cb;
  }

private:
  uint16_t _getRaw()
  {
    return analogRead(_pin);
  }

  uint8_t _getMappedFromRaw(uint16_t raw)
  {
    if (raw > _max)
    {
      _max = raw;
    }
    if (raw < _min)
    {
      _min = raw;
    }

    uint16_t centreLow = _centre - _deadband;
    uint16_t centreHigh = _centre + _deadband;

    bool braking = raw < centreLow;
    bool acceling = raw > centreHigh;

    if (braking)
    {
      return map(raw, _min, centreLow, 0, 127);
    }
    else if (acceling)
    {
      return map(raw, centreHigh, _max, 128, 255);
    }
    return 127;
  }
};
