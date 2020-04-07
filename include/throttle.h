

class ThrottleClass
{
public:
  void init(int pin)
  {
    _pin = pin;
    pinMode(pin, INPUT);

    // _centre = _getRaw();
  }

  uint8_t get(bool accelEnabled)
  {
    uint16_t raw = _getRaw();
    uint8_t mapped = getMappedFromRaw(raw);
#ifdef PRINT_THROTTLE
    DEBUGVAL(raw, mapped);
#endif
    return mapped;
  }

private:
  uint8_t _pin;
  uint16_t _centre = 1946, _min = 240, _max = 4095;
  uint16_t _deadband = 50;

  uint16_t _getRaw()
  {
    return analogRead(_pin);
  }

  uint8_t getMappedFromRaw(uint16_t raw)
  {
    if (raw > _max)
    {
      _max = raw;
    }
    if (raw < _min)
    {
      _min = raw;
    }

    uint16_t centreLow = _centre - _deadband;
    uint16_t centreHigh = _centre + _deadband;

    bool braking = raw < centreLow;
    bool acceling = raw > centreHigh;

    if (braking)
    {
      return map(raw, _min, centreLow, 0, 127);
    }
    else if (acceling)
    {
      return map(raw, centreHigh, _max, 128, 255);
    }
    return 127;
  }
};
