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
  typedef bool (*GetBoolean_Cb)();
  // typedef float (*GetFloat_Cb)();
  typedef uint8_t (*GetUint8_t_Cb)();

  namespace // private
  {
    // uint8_t _accel_direction = DIR_CLOCKWISE;
    uint8_t _throttle = 127;

    GetBoolean_Cb _throttleEnabled_cb = nullptr;
    // GetFloat_Cb _throttleEnabled_cb = nullptr;
    GetUint8_t_Cb _getThrottle_cb = nullptr;
  }

  uint8_t get()
  {
    return _throttle;
  }

  void update(bool force_print = false)
  {
    if (_throttleEnabled_cb == nullptr)
    {
      Serial.printf("ERROR: throttleEnabledCallback is not set!!!\n");
      return;
    }
    if (_getThrottle_cb == nullptr)
    {
      Serial.printf("ERROR: getThrottle is not set!!!\n");
      return;
    }

    _throttle = _getThrottle_cb();

    if (_throttle > 127 && _throttleEnabled_cb() == false)
    {
      _throttle = 127;
    }
  }

  void centre()
  {
  }

  void setThrottleEnabledCb(GetBoolean_Cb cb)
  {
    _throttleEnabled_cb = cb;
  }

  void setGetThrottleCb(GetUint8_t_Cb cb)
  {
    _getThrottle_cb = cb;
  }

  void init(float sweep, float max_delta_limit, float min_delta_limit, uint8_t direction)
  {
    _throttle = 127;
  }

  bool connect()
  {
    return true;
  }
}