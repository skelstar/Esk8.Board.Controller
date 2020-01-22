#ifndef TriggerLib_h
#define TriggerLib_h

#include <Arduino.h>

#define NO_DEADMAN 99

class TriggerLib
{

public:
  TriggerLib(uint8_t trigger_pin, uint8_t deadzone = 0)
  {
    _deadzone = deadzone;
    _pin = trigger_pin;
    deadman_held = true;
  }
//-----------------------------------------------------
  void initialise()
  {
    t_state = IDLE_STATE;
    _centre = get_raw();
    _calibrated = true;
  }
//-----------------------------------------------------
  void set_deadman_pin(uint8_t pin)
  {
    _deadman_pin = pin;
  }
//-----------------------------------------------------
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

//-----------------------------------------------------
  uint8_t get_safe_throttle()
  {
    uint8_t raw = _get_raw_throttle();
    return make_throttle_safe(raw);
  }
//-----------------------------------------------------
  enum TriggerState
  { 
    IDLE_STATE,
    GO_STATE,
    WAIT_STATE
  } t_state;
//-----------------------------------------------------
  void manage_state(uint8_t raw)
  {
    switch(t_state)
    {
      case IDLE_STATE:
        max_throttle = 127;
        if (deadman_held)
        {
          t_state = GO_STATE;
          max_throttle = 255;
        }
        break;
      case GO_STATE:
        max_throttle = 255;
        if (!deadman_held)
        {
          t_state = raw > 127 ? WAIT_STATE : IDLE_STATE;
          max_throttle = raw > 127 ? raw : 127;
        }
        break;
      case WAIT_STATE:
        if (raw <=127) {
          t_state = deadman_held ? GO_STATE : IDLE_STATE;
          max_throttle = t_state == GO_STATE ? 255 : 127;
        }
        else if (raw < max_throttle) {
          max_throttle = raw;
        }
        break;      
    }
  }
//-----------------------------------------------------
  uint8_t make_throttle_safe(uint8_t raw)
  {
    if (_deadman_pin == NO_DEADMAN)
    {
      return raw;
    }

    manage_state(raw);

    uint8_t result = 127;

    switch(t_state)
    {
      case IDLE_STATE:
        result = raw <= max_throttle ? raw : max_throttle;
        break;
      case GO_STATE:
        result = raw;
        break;
      case WAIT_STATE:
        result = max_throttle;
        break;    
      default:
        Serial.printf("Unknown state: %d", t_state);
    }
    return result;
  }
//-----------------------------------------------------

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
