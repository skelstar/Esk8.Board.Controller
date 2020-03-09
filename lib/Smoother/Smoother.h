#define SMOOTHER_H

#ifndef Arduino
#include <Arduino.h>
#endif

class Smoother
{

private:
  struct Reading
  {
    uint8_t val;
    bool blank;
  };

  byte _readingsLen = 10,
       _pos = 0,
       _num = 0;
  Reading *_readings;

public:
  Smoother(byte factor, uint8_t seed, byte numSeed)
  {
    _readingsLen = factor;
    _readings = new Reading[_readingsLen]();

    clear(seed, numSeed);
  }

  void add(uint8_t x)
  {
    if (_num < _readingsLen)
    {
      _num++;
    }
    _readings[_pos].val = x;
    _readings[_pos].blank = false;
    _pos = _pos == _readingsLen - 1 ? 0 : _pos + 1;
  }

  uint8_t get()
  {
    uint16_t runningTotal = 0, runningCount = 0;
    for (int i = 0; i < _readingsLen; i++)
    {
      if (_readings[i].blank == false)
      {
        runningTotal += _readings[i].val;
        runningCount++;
      }
    }
    return runningCount > 0
               ? runningTotal / runningCount
               : 0;
  }

  uint8_t getLast()
  {
    return _pos == 0 ? _readings[_readingsLen - 1].val : _readings[_pos - 1].val;
  }

  uint16_t getIndexed(byte idx)
  {
    return _readings[idx].blank ? 999 : _readings[idx].val;
  }

  void clear(uint8_t seed, byte numSeed)
  {
    for (int i = 0; i < _readingsLen; i++)
    {
      _readings[i].blank = true;
      _readings[i].val = 0;
    }
    _pos = 0;
    // make sure numSeed is less than array size
    for (int j = 0; j < numSeed && j < _readingsLen; j++)
    {
      _readings[j].val = seed;
      _readings[j].blank = false;
      _pos++;
    }
  }

  void printBuffer()
  {
    Serial.printf("[");
    for (int i = 0; i < _readingsLen; i++)
    {
      if (!_readings[i].blank)
      {
        Serial.printf("%d", getIndexed(i));
      }
      else
      {
        Serial.printf("--");
      }
      if (i < _readingsLen - 1)
        Serial.printf(", ");
    }
    Serial.printf("] \n");
  }
};