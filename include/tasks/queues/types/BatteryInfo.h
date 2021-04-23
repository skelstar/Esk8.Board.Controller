#pragma once

#include <Arduino.h>

#include <tasks/queues/types/QueueBase.h>

class BatteryInfo : public QueueBase
{
public:
  bool charging;
  float percent;
  float volts;

  void print(const char *preamble)
  {
    Serial.printf(" %s Batt charging: %s, percent: %1f, volts: %.1f\n",
                  preamble, charging ? "YES" : "NO", percent, volts);
  }
};