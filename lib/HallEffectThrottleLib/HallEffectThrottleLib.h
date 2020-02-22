#ifndef HALLEFFECTTHROTTLELIB_H

#ifndef Arduino
#include <Arduino.h>
#endif

#define HALL_TRIGGER_MAX_PROVISIONAL 2500
// #define HALL_TRIGGER_MID_PROVISIONAL 1830
#define HALL_TRIGGER_MIN_PROVISIONAL 1300
#define HALL_TRIGGER_DEADBAND 10

class HallEffectThrottleLib
{

private:
  uint8_t _pin;
  uint16_t _raw,
      _mapped;
  uint16_t _lowest = HALL_TRIGGER_MIN_PROVISIONAL;
  uint16_t _middle = 0;
  uint16_t _highest = HALL_TRIGGER_MAX_PROVISIONAL;
  uint8_t _throttle = 127;
  uint8_t _steps, _steps_braking = 10, _steps_accel = 10;

  void _updateLimits(uint16_t raw)
  {
    if (_lowest > raw)
    {
      _lowest = raw;
      Serial.printf("_lowest: %d\n", raw);
    }
    if (_highest < raw)
    {
      _highest = raw;
      Serial.printf("_highest: %d\n", raw);
    }
  }

public:
  void init(uint8_t pin, uint8_t steps_braking, uint8_t steps_accel)
  {
    _pin = pin;
    _steps_braking = steps_braking;
    _steps_accel = steps_accel;

    pinMode(_pin, INPUT);
    _lowest = HALL_TRIGGER_MIN_PROVISIONAL;
    _highest = HALL_TRIGGER_MAX_PROVISIONAL;
  }

  uint8_t getThrottle(bool moving)
  {
    if (!moving)
    {
      return 127;
    }

    _raw = analogRead(_pin);

    _updateLimits(_raw);

    uint16_t constrained = constrain(_raw, _lowest, _highest);

    _throttle = 127;
    if (constrained >= _middle + HALL_TRIGGER_DEADBAND)
    {
      _steps = map(constrained, _middle, _highest, 0, _steps_accel);
      _throttle = map(_steps, 0, _steps_accel, 127, 255);
    }
    else if (constrained <= _middle - HALL_TRIGGER_DEADBAND)
    {
      _steps = map(constrained, _lowest, _middle, _steps_braking, 0);
      _throttle = map(_steps, _steps_braking, 0, 0, 127);
    }

    return _throttle;
  }

  void getMiddle()
  {
    uint16_t summed_mid = 0;
    for (uint8_t i = 0; i < 5; i++)
    {
      summed_mid += analogRead(_pin);
      delay(50);
    }
    _middle = summed_mid / 5;
    Serial.printf("_middle: %d\n", _middle);
  }

  void print()
  {
    bool braking = _throttle < 127;
    bool accel = _throttle > 127;

    // brake
    for (uint8_t i = 0; i < _steps_braking; i++)
    {
      Serial.printf("%s", i >= (_steps_braking - _steps) && braking ? "X" : "-");
    }
    // middle
    Serial.printf("+");
    // acccel
    for (uint8_t i = 0; i < _steps_accel; i++)
    {
      Serial.printf("%s", i < _steps && accel ? "X" : "-");
    }
    Serial.printf("    -> %d -> %d -> %d\n", _raw, _steps, _throttle);
  }
};

#endif