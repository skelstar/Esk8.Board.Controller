#ifndef ARDUINO_H
#include <Arduino.h>
#include <rom/rtc.h> // for reset reason
#endif

class Stats
{
  typedef void (*ResetsAcknowledgedCallback)();

public:
  uint16_t total_failed_sending;
  unsigned long consecutive_resps;
  RESET_REASON reset_reason_core0;
  RESET_REASON reset_reason_core1;
  uint16_t soft_resets = 0;
  uint8_t boardResets = 0;
  unsigned long timeMovingMS = 0;

  void setResetsAcknowledgedCallback(ResetsAcknowledgedCallback cb)
  {
    _resetsAcknowledgedCallback = cb;
  }

  bool needToAckResets()
  {
    return soft_resets > 0 && !_resetsAcknowledged;
  }

  void ackResets()
  {
    _resetsAcknowledged = true;
    soft_resets = 0;
    _resetsAcknowledgedCallback();
  }

  float getSecondsMoving()
  {
    return timeMovingMS / 1000.0;
  }

  float getAverageAmpHoursPerSecond(float amphours)
  {
    return timeMovingMS > 0
               ? amphours / getSecondsMoving()
               : 0;
  }

private:
  bool _resetsAcknowledged = false;
  ResetsAcknowledgedCallback _resetsAcknowledgedCallback;
};