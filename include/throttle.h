

typedef void (*ThrottleChangedCallback)(uint8_t);

class ThrottleClass
{
public:
  void init(int pin, ThrottleChangedCallback throttleChangedCb)
  {
    _pin = pin;
    _throttleChangedCb = throttleChangedCb;
    pinMode(pin, INPUT);
    _oldMapped = 127;
  }

  uint8_t get(bool enabled = true)
  {
    if (enabled)
    {
      _raw = _getRaw();
      uint8_t mapped = getMappedFromRaw();
      if (_oldMapped != mapped && _throttleChangedCb != nullptr)
        _throttleChangedCb(mapped);
      if (PRINT_THROTTLE && _oldMapped != mapped)
        DEBUGVAL(_raw, mapped);
      _oldMapped = mapped;
      return mapped;
    }
    else
    {
      return 127;
    }
  }

private:
  uint8_t _pin;
  uint16_t _raw;
  uint8_t _oldMapped;
  uint16_t _centre = 1280, _min = 0, _max = 2530;
  uint16_t _deadband = 50;
  ThrottleChangedCallback _throttleChangedCb = nullptr;

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
};
