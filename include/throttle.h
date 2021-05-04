
#include <FastMap.h>

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

    uint16_t centreLow = THROTTLE_RAW_CENTRE - (THROTTLE_RAW_DEADBAND / 2);
    uint16_t centreHigh = THROTTLE_RAW_CENTRE + (THROTTLE_RAW_DEADBAND / 2);

    _accelmapper.init(centreHigh, THROTTLE_RAW_MAX, 128, 255);
    _brakemapper.init(THROTTLE_RAW_MIN, centreLow, 0, 127);
  }

  uint8_t get(bool enabled = true)
  {
    if (enabled)
    {
      _raw = _getRaw();
      uint8_t mapped = _getMappedFromRaw();
      if (_oldMapped != mapped && _throttleChangedCb != nullptr)
        _throttleChangedCb(mapped);
      if (PRINT_THROTTLE && _oldMapped != mapped)
        Serial.printf("raw: %d mapped: %d\n", _raw, mapped);
      _oldMapped = mapped;
      return mapped;
    }
    else
    {
      return 127;
    }
  }

  uint16_t getRaw()
  {
    return _raw;
  }

private:
  uint8_t _pin;
  uint16_t _raw;
  uint8_t _oldMapped;
  ThrottleChangedCallback _throttleChangedCb = nullptr;
  FastMap _accelmapper, _brakemapper;

  uint16_t _getRaw()
  {
    return analogRead(_pin);
  }

  uint8_t _getMappedFromRaw()
  {
    uint16_t centreLow = THROTTLE_RAW_CENTRE - THROTTLE_RAW_DEADBAND;
    uint16_t centreHigh = THROTTLE_RAW_CENTRE + THROTTLE_RAW_DEADBAND;

    bool braking = _raw < centreLow;
    bool acceling = _raw > centreHigh;

    if (braking)
    {
      return _brakemapper.constrainedMap(_raw);
    }
    else if (acceling)
    {
      return _accelmapper.constrainedMap(_raw);
    }
    return 127;
  }
};
