

class ThrottleClass
{
public:
  void init(int pin)
  {
    _pin = pin;
    pinMode(pin, INPUT);
    _oldMapped = 127;
    _state = NORMAL;
  }

  uint8_t get(bool accelEnabled)
  {
    _raw = _getRaw();
    uint8_t mapped = getMappedFromRaw();

#ifdef THROTTLE_PROTECTION
    if (throttleIsDangerous(mapped))
    {
      return 127;
    }
#endif

#ifdef PRINT_THROTTLE
    DEBUGVAL(_raw, mapped);
#endif
    _oldMapped = mapped;
    return mapped;
  }

private:
  enum State
  {
    NORMAL,
    DANGEROUS_THROTTLE,
  };

  State _state;

  uint8_t _pin;
  uint16_t _raw;
  uint8_t _oldMapped;
  uint16_t _centre = 1946, _min = 240, _max = 4095;
  uint16_t _deadband = 50;

  uint16_t _getRaw()
  {
    return analogRead(_pin);
  }

  uint8_t getMappedFromRaw()
  {
    if (_raw > _max)
    {
      _max = _raw;
    }
    if (_raw < _min)
    {
      _min = _raw;
    }

    uint16_t centreLow = _centre - _deadband;
    uint16_t centreHigh = _centre + _deadband;

    bool braking = _raw < centreLow;
    bool acceling = _raw > centreHigh;

    if (braking)
    {
      return map(_raw, _min, centreLow, 0, 127);
    }
    else if (acceling)
    {
      return map(_raw, centreHigh, _max, 128, 255);
    }
    return 127;
  }

  bool throttleIsDangerous(uint8_t mapped)
  {
    uint8_t diff = abs(_oldMapped - mapped);
    bool dangerousAccel = _oldMapped < mapped &&
                          mapped > 128 &&
                          diff > THROTTLE_PROTECTION;
    bool dangerousBraking = _oldMapped > mapped &&
                            mapped < 127 &&
                            diff > THROTTLE_PROTECTION;

    switch (_state)
    {
    case NORMAL:
      if (dangerousAccel || dangerousBraking)
      {
        DEBUGVAL("-> DANGEROUS_THROTTLE", _oldMapped, mapped);
        _state = DANGEROUS_THROTTLE;
        return true;
      }
      break;
    case DANGEROUS_THROTTLE:
      if (getMappedFromRaw() == 127)
      {
        DEBUGVAL("-> NORMAL", _oldMapped, mapped);
        _oldMapped = mapped;
        _state = NORMAL;
      }
      return true;
    }
    return false;
  }
};
