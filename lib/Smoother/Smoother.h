#define SMOOTHER_H

#ifndef Arduino
#include <Arduino.h>
#endif

class Smoother
{
private:
  byte _factor = 10,
       _pos = 0,
       _num = 0;
  uint8_t *_readings;

public:
  Smoother(byte factor, uint8_t seed, byte numSeed)
  {
    _factor = factor;
    _readings = new uint8_t[_factor]();

    for (int i = 0; i < _factor; i++)
    {
      _readings[i] = NULL;
    }
    clear(seed, numSeed);
  }

  void add(uint8_t x)
  {
    if (_num < _factor)
    {
      _num++;
    }
    _readings[_pos] = x;
    _pos = _pos == _factor - 1 ? 0 : _pos + 1;
  }

  uint8_t get()
  {
    uint16_t runningTotal = 0, runningCount = 0;
    for (int i = 0; i < _factor; i++)
    {
      if (_readings[i] != NULL)
      {
        runningTotal += _readings[i];
        runningCount++;
      }
    }
    return runningCount > 0
               ? runningTotal / runningCount
               : 0;
  }

  uint8_t getLast()
  {
    return _pos == 0 ? _readings[_factor - 1] : _readings[_pos];
  }

  void clear(uint8_t seed, byte numSeed)
  {
    for (int i = 0; i < _factor; i++)
    {
      _readings[i] = NULL;
    }
    for (int j = 0; j < numSeed; j++)
    {
      _readings[j] = seed;
    }
  }
};