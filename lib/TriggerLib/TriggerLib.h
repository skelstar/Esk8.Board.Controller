#ifndef TriggerLib_h
#define TriggerLib_h

#include <Arduino.h>

#define NO_DEADMAN  99

class TriggerLib
{

public:
  TriggerLib(uint8_t trigger_pin, uint8_t deadzone = 0)
  {
    _deadzone = deadzone;
    _pin = trigger_pin;
    deadman_held = true;
  }

  void initialise()
  {
    _centre = get_raw();
    _calibrated = true;
  }

  void set_deadman_pin(uint8_t pin)
  {
    _deadman_pin = pin;
  }


  uint8_t _get_raw_throttle()
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

  uint8_t get_safe_throttle()
  {
    uint8_t raw = _get_raw_throttle();
    return make_throttle_safe(raw);
  }

  uint8_t make_throttle_safe(uint8_t raw)
  {
    if (_deadman_pin == NO_DEADMAN)    
    {
      return raw;
    }
    bool braking_or_idle = raw <= 127;
    uint8_t result = 127;

    if (waiting_for_idle_throttle && braking_or_idle)
    {
      waiting_for_idle_throttle = false;
    }

    if (deadman_held)
    {
      result = raw;
      if (waiting_for_idle_throttle)
      {
        result = raw <= max_throttle
                     ? raw
                     : max_throttle;
        max_throttle = raw;
      }
    }
    else
    {
      result = braking_or_idle ? raw
                               : 127;
    }
    return result;
  }

  bool deadman_held;
  bool waiting_for_idle_throttle;
  uint8_t max_throttle;

private:
  uint16_t get_raw()
  {
    return analogRead(_pin);
  }

  uint8_t _deadman_pin = NO_DEADMAN;
  uint16_t _centre = 0;
  uint16_t max = 0;
  uint16_t min = 0;
  bool _calibrated = false;
  bool _can_accelerate = true;
  uint8_t _deadzone; // either side of centre
  uint8_t _pin;
};

#endif
