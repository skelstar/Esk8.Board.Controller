#ifndef Arduino
#include <Arduino.h>
#endif

class BatteryLib
{
  typedef void (*BatteryValueChangedCallback)();

public:
  BatteryLib(uint8_t pin, uint8_t gnd_pin = 99)
  {
    _pin = pin;
    _gnd_pin = gnd_pin;
  }

  void setup(BatteryValueChangedCallback cb, uint8_t rounding = 100)
  {
    _battery_value_changed_cb = cb;
    _rounding = rounding;
    pinMode(_pin, INPUT);
    if (_gnd_pin != 99)
    {
      pinMode(_gnd_pin, OUTPUT);
      digitalWrite(_gnd_pin, LOW);
    }
    vTaskDelay(10);
    read_remote_battery();
  }

  void read_remote_battery()
  {
    uint16_t remote_battery_volts_raw = analogRead(_pin);
    remote_battery_percent = get_remote_battery_percent(remote_battery_volts_raw);
    if (remote_battery_percent != _old_remote_battery_percent)
    {
      _old_remote_battery_percent = remote_battery_percent;
      if (_battery_value_changed_cb != NULL)
      {
        _battery_value_changed_cb();
      }
    }
  }

  uint8_t remote_battery_percent = 0;

private:
#define REMOTE_BATTERY_FULL 2300
#define REMOTE_BATTERY_EMPTY 1520

  uint8_t get_remote_battery_percent(uint16_t raw_battery)
  {
    uint16_t numerator = raw_battery > REMOTE_BATTERY_EMPTY
                             ? raw_battery < REMOTE_BATTERY_FULL
                                   ? raw_battery - REMOTE_BATTERY_EMPTY
                                   : REMOTE_BATTERY_FULL - REMOTE_BATTERY_EMPTY
                             : 1;

    uint16_t denominator = REMOTE_BATTERY_FULL - REMOTE_BATTERY_EMPTY;
    float ratio = numerator / (denominator * 1.0);
    // make low_poly
    return (uint8_t)((ratio * 100) / _rounding) * _rounding;
  }

  BatteryValueChangedCallback _battery_value_changed_cb;
  uint8_t _pin, _gnd_pin, _rounding;
  uint8_t _old_remote_battery_percent = 0;
};