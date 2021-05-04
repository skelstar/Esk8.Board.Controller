#include <Arduino.h>

class ThrottleProviderBase
{
public:
  void init();
  uint8_t get();
  void update();
  bool withinLimits();
};