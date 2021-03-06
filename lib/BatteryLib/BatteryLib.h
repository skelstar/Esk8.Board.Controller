#pragma once

#include <Arduino.h>
#include <FastMap.h>

class BatteryLib
{
  typedef void (*BatteryValueChangedCallback)();

#define REMOTE_BATTERY_FULL 2300
#define REMOTE_BATTERY_EMPTY 1520

public:
  BatteryLib(uint8_t pin)
  {
    _pin = pin;

    _mapper.init(REMOTE_BATTERY_EMPTY, REMOTE_BATTERY_FULL, 0, 10);
    _voltsMapper.init(REMOTE_BATTERY_EMPTY, REMOTE_BATTERY_FULL, 3.4, 4.2);
  }

  void setup(BatteryValueChangedCallback cb)
  {
    _battery_value_changed_cb = cb;
    pinMode(_pin, INPUT);
    vTaskDelay(10);
    update();
  }

  void update()
  {
    _rawVolts = analogRead(_pin);
    chargePercent = _get_percent(_rawVolts);
    if (_battery_value_changed_cb != NULL)
      _battery_value_changed_cb();
  }

  bool isCharging = false;

  uint16_t rawAnalogCount()
  {
    return _rawVolts;
  }

  float getVolts()
  {
    return _voltsMapper.constrainedMap(_rawVolts);
  }

public:
  uint8_t chargePercent = 0;

private:
  uint8_t _get_percent(uint16_t raw_battery)
  {
    isCharging = raw_battery > REMOTE_BATTERY_FULL + 100;

    return 10 * _mapper.constrainedMap(raw_battery);
  }

private:
  BatteryValueChangedCallback _battery_value_changed_cb;
  uint8_t _pin;
  uint16_t _rawVolts;
  FastMap _mapper, _voltsMapper;
};