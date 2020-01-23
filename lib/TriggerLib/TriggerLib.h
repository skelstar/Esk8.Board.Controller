#ifndef TriggerLib_h
#define TriggerLib_h

#include <Arduino.h>

class TriggerLib
{

public:
  TriggerLib(uint8_t pin, uint8_t deadzone = 0)
  {
    _deadzone = deadzone;
    _pin = pin;
  }

  void initialise()
  {
    _centre = get_raw();
    _calibrated = true;
  }

  uint8_t get_throttle()
  {
    if (_calibrated == false)
    {
      return 127;
    }

    uint16_t raw = get_raw();

    uint16_t _centre_upper = _centre + _deadzone;
    uint16_t _centre_lower = _centre - _deadzone;

    if (raw > _centre_upper)
    {
      return map(raw, _centre_upper, 4096, 127, 255);
    }
    else if (raw < _centre_lower)
    {
      return map(raw, 0, _centre_lower, 0, 127);
    }
    return 127;
  }

  uint8_t static get_push_to_start_throttle(uint8_t raw, bool moving)
  {
    return moving || raw <= 127 ? raw : 127;
  }

private:
  uint16_t get_raw()
  {
    return analogRead(_pin);
  }

  uint16_t _centre = 0;
  uint16_t max = 0;
  uint16_t min = 0;
  bool _calibrated = false;
  bool _can_accelerate = true;
  uint8_t _deadzone; // either side of centre
  uint8_t _pin;
};

#endif
