#define OPTIONVAL_H

#ifndef Arduino
#include <Arduino.h>
#endif

class OptionValue
{
public:
  OptionValue(int16_t min, int16_t max, int16_t current, uint8_t step)
  {
    _min = min;
    _max = max;
    _current = current;
    _step = step;
  }

  void up()
  {
    _current += _step;
    _current = constrain(_current, _min, _max);
  }

  void dn()
  {
    _current -= _step;
    _current = constrain(_current, _min, _max);
  }

  int16_t get()
  {
    return _current;
  }

private:
  int16_t _min, _max, _current;
  uint8_t _step;
};