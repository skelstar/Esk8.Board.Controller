#pragma once

#ifndef Arduino
#include <Arduino.h>
#endif

#include <Wire.h>
#include <FastMap.h>
#include <Fsm.h>
#include <QueueManager.h>
#include <sharedConstants.h>

#include "ADS1X15.h"

ADS1115 ADS(0x0248);

//=========================================

typedef void (*VoidUint8Callback)(uint8_t);
typedef bool (*GetBoolean_Cb)();

class AnalogI2CTriggerClass
{
public:
  bool printThrottle = false;

  enum ReturnCode
  {
    OK = 0,
    I2C_ADC_NOT_FOUND,
    RAW_OUT_OF_RANGE,
  };

private:
  VoidUint8Callback _throttleChangedCb = nullptr;
  GetBoolean_Cb _throttleEnabled_cb = nullptr;

  uint8_t _accel_direction = DIR_CLOCKWISE;
  FastMap _accelmapper, _brakemapper;
  uint8_t _oldMapped = 0, _throttle = 127;
  uint16_t _raw,
      _centre = THROTTLE_RAW_CENTRE,
      _min = THROTTLE_RAW_MIN,
      _max = THROTTLE_RAW_MAX,
      _deadband = THROTTLE_RAW_DEADBAND;
  SemaphoreHandle_t _i2c_mux = nullptr;

public:
  AnalogI2CTriggerClass()
  {
  }

#include <utils.h>

  void init(SemaphoreHandle_t i2c_mux, uint8_t accelDirection = DIR_CLOCKWISE)
  {
    _i2c_mux = i2c_mux;
    _accel_direction = accelDirection;
    assert(_i2c_mux != nullptr);
    assert(_throttleEnabled_cb != nullptr);
    assert(_accel_direction == DIR_CLOCKWISE || _accel_direction == DIR_ANTI_CLOCKWISE);
    _oldMapped = 127;

    if (take(_i2c_mux, TICKS_500ms))
    {
      while (!ADS.begin())
      {
        vTaskDelay(TICKS_100ms);
        Serial.printf("Trying to connect to trigger i2c\n");
      }
      ADS.setGain(0);

      Serial.printf("%s\n", ADS.isConnected() ? "Connected" : "disconnected");
      give(_i2c_mux);
    }
    else
    {
      Serial.printf("Couldn't take mutex (AnalogI2CTrigger\n");
    }

    centre();
  }

  uint8_t get()
  {
    return _throttle;
  }

  ReturnCode update(bool enabled = true)
  {
    if (enabled)
    {
      _raw = _getRaw();

      if (!_rawIsSafe(_raw))
      {
        _throttle = 127;
        return ReturnCode::RAW_OUT_OF_RANGE;
      }

      _throttle = _getMappedFromRaw(_raw);

      if (_oldMapped != _throttle && _throttleChangedCb != nullptr)
        _throttleChangedCb(_throttle);
      if (printThrottle && _oldMapped != _throttle)
        Serial.printf("raw: %d centre: %d mapped: %d\n", _raw, _centre, _throttle);
      _oldMapped = _throttle;
      return ReturnCode::OK;
    }
    _throttle = 127;
    return ReturnCode::OK;
  }

  void centre()
  {
    _raw = _getRaw();
    _centre = _raw;
    _throttle = _getMappedFromRaw(_raw);
    Serial.printf("Centred trigger: %d\n", _centre);
  }

  void setThrottleEnabledCb(GetBoolean_Cb cb)
  {
    _throttleEnabled_cb = cb;
  }

  // connect i2c, use mux
  bool connect()
  {
    return true;
  }

private:
  bool _rawIsSafe(uint16_t raw)
  {
    return _raw > _min - 2000 && _raw < _max + 2000;
  }

  uint16_t _getRaw()
  {
    if (take(_i2c_mux, TICKS_100ms))
    {
      give(_i2c_mux);
      return ADS.readADC(0);
    }
    else
    {
      Serial.printf("couldn't take mutex\n");
    }
    return _centre;
  }

  uint8_t _getMappedFromRaw(uint16_t raw)
  {
    // calibrate
    _max = raw > _max ? raw : _max;
    _min = raw < _min ? raw : _min;

    uint16_t centreLow = _centre;
    uint16_t centreHigh = _centre;
    bool braking = false;
    bool acceling = false;

    centreLow = _centre - _deadband;
    centreHigh = _centre + _deadband;

    if (_accel_direction == DIR_PULL_TO_ACCEL)
    {
      // 1050 -> 8200 -> 15100
      // _min   _centre  _max
      // 255     127     0
      // pushing trigger is accelerating
      braking = raw > centreHigh;
      acceling = raw < centreLow;
      return braking    ? map(raw, _max, centreHigh, 0, 127)
             : acceling ? map(raw, centreLow, _min, 128, 255)
                        : 127;
    }
    else if (_accel_direction == DIR_PUSH_TO_ACCEL)
    {
      // 1050 -> 8200 -> 15100
      // _min   _centre  _max
      // 0        127     255
      // pulling trigger is accelerating
      braking = raw < centreLow;
      acceling = raw > centreHigh;
      return braking    ? map(raw, _min, centreLow, 0, 127)
             : acceling ? map(raw, centreHigh, _max, 128, 255)
                        : 127;
    }
    return 127;
  }
};
