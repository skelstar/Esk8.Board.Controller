#ifndef Arduino
#include <Arduino.h>
#endif

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
  }

  void setup(BatteryValueChangedCallback cb)
  {
    _battery_value_changed_cb = cb;
    pinMode(_pin, INPUT);
    vTaskDelay(10);
    read_remote_battery();
  }

  void read_remote_battery()
  {
    _rawVolts = analogRead(_pin);
    chargePercent = _get_remote_battery_percent(_rawVolts);
    // removed logic comparing old to latest batt volts
    _battery_value_changed_cb();
  }

  bool isCharging = false;

  uint16_t rawAnalogCount()
  {
    return _rawVolts;
  }

public:
  uint8_t chargePercent = 0;

private:
  uint8_t _get_remote_battery_percent(uint16_t raw_battery)
  {
    isCharging = raw_battery > REMOTE_BATTERY_FULL + 100;

    // int limited = constrain(raw_battery, REMOTE_BATTERY_EMPTY, REMOTE_BATTERY_FULL);

    // should return 0, 10, 20 ... 100

    return 10 * _mapper.constrainedMap(raw_battery);
    //  map(limited,
    //                 REMOTE_BATTERY_EMPTY,
    //                 REMOTE_BATTERY_FULL,
    //                 0,
    //                 10);
  }

private:
  BatteryValueChangedCallback _battery_value_changed_cb;
  uint8_t _pin;
  uint16_t _rawVolts;
  FastMap _mapper;
};