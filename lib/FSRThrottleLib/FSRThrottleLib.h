#ifndef Arduino
#include <Arduino.h>
#endif

#ifndef Smoothed
#include <Smoothed.h>
#endif

#ifndef BUtton2
#include <Button2.h>
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
  FSRThrottleLib(FSRPin *accelPin, FSRPin *brakePin, Button2 *deadman)
  {
    _accelPin = accelPin;
    _brakePin = brakePin;
    _deadman = deadman;
  }

  void init()
  {
    _brakePin->init();
    _accelPin->init();
  }

  uint8_t get()
  {
    uint8_t brakeVal, accelVal;

#ifdef USE_DEADMAN
    _deadman->loop();
    if (_deadman->isPressed() == false)
    {
      return 127;
    }
#endif

    brakeVal = _brakePin->get();
    accelVal = _accelPin->get();

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
      Serial.printf("--------------------");
    }
    else
    {
      Serial.printf("--------------------");
      uint8_t printMapped = map(throttle, 127, 255, 0, width);
      for (uint8_t i = 0; i <= width; i++)
      {
        Serial.printf("%s", i <= printMapped ? "#" : "-");
      }
    }
    Serial.printf(" : %03d br: %04d acc: %04d\n", throttle, _brakePin->getLastRaw(), _accelPin->getLastRaw());
  }

private:
  FSRPin *_accelPin, *_brakePin;
  Button2 *_deadman;
  uint8_t throttle = 127;
};