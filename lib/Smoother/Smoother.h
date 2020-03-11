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
       _pos = 0;
  Reading *_readings;

public:
  Smoother(byte factor, uint8_t seed)
  {
    _readingsLen = factor;
    _readings = new Reading[_readingsLen]();

    clear(seed, _readingsLen);
  }

  void add(uint8_t x)
  {
    _pos = _pos < _readingsLen - 1
               ? _pos + 1
               : 0;
    _readings[_pos].val = x;
    _readings[_pos].blank = false;
    // Serial.printf("adding %d at %d\n", x, _pos);
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
               ? (uint8_t)(runningTotal / runningCount)
               : 0;
  }

  uint8_t getLast()
  {
    return _readings[_pos].val;
  }

  uint16_t getIndexed(byte idx)
  {
    return _readings[idx].blank ? 999 : _readings[idx].val;
  }

  void clear(uint8_t seed)
  {
    clear(seed, _readingsLen);
  }

  void clear(uint8_t seed, byte numSeed)
  {
    if (numSeed > _readingsLen)
    {
      numSeed = _readingsLen;
    }

    for (int i = 0; i < _readingsLen; i++)
    {
      _readings[i].blank = true;
      _readings[i].val = 0;
    }
    _pos = 0;
    // make sure numSeed is less than array size
    for (int j = 0; j < numSeed; j++)
    {
      _readings[j].val = seed;
      _readings[j].blank = false;
    }
    _pos = numSeed - 1;
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