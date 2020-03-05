#ifndef Arduino
#include <Arduino.h>
#endif

class FSRPin
{
public:
  FSRPin() {}
  FSRPin(uint8_t pin,
         uint16_t min,
         uint16_t max,
         uint8_t mapMin,
         uint8_t mapMax,
         uint8_t *inMap,
         uint8_t *outMap)
  {
    _pin = pin;
    _min = min;
    _max = max;
    _mapMin = mapMin;
    _mapMax = mapMax;
    _in = inMap;
    _out = outMap;
  }

  void init()
  {
    pinMode(_pin, INPUT);
  }

  uint8_t get()
  {
    uint16_t raw = constrain(analogRead(_pin), _min, _max);
    uint8_t mapped = map(raw, _min, _max, _mapMin, _mapMax);
    return _multiMap(mapped);
  }

private:
  uint8_t _pin;
  uint16_t _min, _max;
  uint8_t _mapMin, _mapMax;
  uint8_t *_in;
  uint8_t *_out;

  // note: the _in array should have increasing values
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
  FSRThrottleLib(FSRPin accelPin, FSRPin brakePin)
  {
    _accelPin = accelPin;
    _brakePin = brakePin;
  }

  void init()
  {
  }

  uint8_t get()
  {
    uint8_t brakeVal, accelVal;
    brakeVal = _brakePin.get();
    accelVal = _accelPin.get();

    if (brakeVal < 127)
    {
      throttle = brakeVal;
    }
    else
    {
      throttle = accelVal;
    }
    return throttle;
  }

  void print(uint8_t width)
  {
    if (throttle < 127)
    {
      uint8_t printMapped = map(throttle, 0, 127, 0, width);
      for (uint8_t i = 0; i <= width; i++)
      {
        Serial.printf("%s", i < printMapped ? "-" : "#");
      }
      Serial.printf("--------------------\n");
    }
    else
    {
      Serial.printf("--------------------");
      uint8_t printMapped = map(throttle, 127, 255, 0, width);
      for (uint8_t i = 0; i <= width; i++)
      {
        Serial.printf("%s", i <= printMapped ? "#" : "-");
      }
      Serial.println();
    }
  }

private:
  FSRPin _accelPin, _brakePin;
  uint8_t throttle = 127;
};