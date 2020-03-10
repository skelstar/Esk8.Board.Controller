#ifndef Arduino
#include <Arduino.h>
#endif

#ifndef Smoother
#include <Smoother.h>
#endif

class FSRPin
{
public:
  FSRPin() {}
  FSRPin(uint8_t pin,
         uint16_t min,
         uint16_t max,
         uint8_t mapMin,
         uint8_t mapMax)
  {
    _pin = pin;
    _min = min;
    _max = max;
    _mapMin = mapMin;
    _mapMax = mapMax;
  }

  void init()
  {
    pinMode(_pin, INPUT);
  }

  void setMaps(uint8_t in[], uint8_t out[])
  {
    bool validMaps = _in[0] != _in[1] && _out[0] != _out[1];
    if (!validMaps)
    {
      DEBUG("ERROR: maps not valid!");
      return;
    }
    for (uint8_t i = 0; i < 4; i++)
    {
      _in[i] = in[i];
      _out[i] = out[i];
    }
  }

  uint8_t get()
  {
    bool validMaps = _in[0] != _in[1] && _out[0] != _out[1];
    _lastRaw = constrain(analogRead(_pin), _min, _max);
    uint8_t mapped = map(_lastRaw, _min, _max, _mapMin, _mapMax);
    return validMaps ? _multiMap(mapped) : mapped;
  }

  int16_t getLastRaw()
  {
    return _lastRaw;
  }

private:
  uint8_t _pin;
  uint16_t _min, _max, _lastRaw;
  uint8_t _mapMin, _mapMax;
  uint8_t _in[4], _out[4];

  uint8_t _multiMap(uint8_t val)
  {
    uint8_t size = sizeof(_in);
    // take care the value is within range
    if (val <= _in[0])
    {
      return _out[0];
    }
    if (val >= _in[size - 1])
    {
      return _out[size - 1];
    }
    // search right interval
    uint8_t pos = 1; // _in[0] allready tested
    while (val > _in[pos])
    {
      pos++;
    }
    // this will handle all exact "points" in the _in array
    if (val == _in[pos])
    {
      return _out[pos];
    }
    // interpolate in the right segment for the rest
    return (val - _in[pos - 1]) * (_out[pos] - _out[pos - 1]) / (_in[pos] - _in[pos - 1]) + _out[pos - 1];
  }
};

class FSRThrottleLib
{
public:
  Smoother *accelSmoother;
  Smoother *brakeSmoother;

  FSRThrottleLib(FSRPin *accelPin, FSRPin *brakePin)
  {
    _accelPin = accelPin;
    _brakePin = brakePin;

    accelSmoother = new Smoother(/*factor*/ 5, /*seed*/ 127);
    brakeSmoother = new Smoother(/*factor*/ 5, /*seed*/ 127);
  }

  void init()
  {
    _brakePin->init();
    _accelPin->init();
  }

  /* set the number of values to smooth over */
  void set(byte factor)
  {
    accelSmoother = new Smoother(factor, /*seed*/ 127);
  }

  /* get the smoothed value */
  uint8_t get(bool accelEnabled)
  {
    uint8_t brakeVal, accelVal;

    brakeVal = _brakePin->get();
    accelVal = _accelPin->get();

    if (brakeVal < 127)
    {
      _throttle = _getBrakingThrottle(brakeVal);
    }
    else if (accelVal > 127 && accelEnabled)
    {
      _throttle = _getAccelThrottle(accelVal, accelEnabled);
    }
    else
    {
      _throttle = _getIdleThrottle();
    }
    return _throttle;
  }

  enum FSRMode
  {
    IDLE,
    ACCEL,
    BRAKE
  };

  void setSmoothing(FSRMode mode, byte factor)
  {
    if (mode == ACCEL)
    {
      accelSmoother = new Smoother(/*factor*/ factor, /*seed*/ 127);
    }
    else if (mode == BRAKE)
    {
      brakeSmoother = new Smoother(/*factor*/ factor, /*seed*/ 127);
    }
  }

  void print(uint8_t width, uint16_t thrToShow = 999)
  {
    if (thrToShow == 999)
    {
      thrToShow = _throttle;
    }
    if (thrToShow < 127)
    {
      uint8_t printMapped = map(thrToShow, 0, 127, 0, width);
      for (uint8_t i = 0; i <= width; i++)
      {
        Serial.printf("%s", i < printMapped ? "-" : "#");
      }
      Serial.printf("--------------------");
    }
    else
    {
      Serial.printf("--------------------");
      uint8_t printMapped = map(thrToShow, 127, 255, 0, width);
      for (uint8_t i = 0; i <= width; i++)
      {
        Serial.printf("%s", i <= printMapped ? "#" : "-");
      }
    }
    Serial.printf(" : %03d br: %04d acc: %04d\n", thrToShow, _brakePin->getLastRaw(), _accelPin->getLastRaw());
  }

private:
  FSRPin *_accelPin, *_brakePin;
  uint8_t _throttle = 127, _last_throttle = 127;
  //----------------
  uint8_t _getBrakingThrottle(uint8_t val)
  {
    uint8_t t = val;
#ifdef USE_THROTTLE_SMOOTHING
    accelSmoother->clear(/*seed*/ 127, /*numSeed*/ 3);
    brakeSmoother->add(val);
    t = brakeSmoother->get();
#endif
    _last_throttle = t;
    return t;
  }
  //----------------

  uint8_t _getAccelThrottle(uint8_t val, bool accelEnabled)
  {
    uint8_t t = val;
#ifdef USE_THROTTLE_SMOOTHING
    brakeSmoother->clear(/*seed*/ 127);
    accelSmoother->add(val);
    t = accelSmoother->get();
#endif
    _last_throttle = t;
    return t;
  }
  //----------------

  uint8_t _getIdleThrottle()
  {
#ifdef USE_THROTTLE_SMOOTHING
    brakeSmoother->clear(/*seed*/ 127);
    accelSmoother->clear(/*seed*/ 127, 3);
#endif
    return 127;
  }
};