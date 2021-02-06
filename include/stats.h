#ifndef ARDUINO_H
#include <Arduino.h>
#include <rom/rtc.h> // for reset reason
#endif
#ifndef STORAGE_H
#include <Storage.h>
#endif

class Stats
{
  typedef void (*ResetsAcknowledgedCallback)();

public:
  uint16_t total_failed_sending;
  unsigned long consecutive_resps;
  uint16_t soft_resets = 0;
  uint8_t boardResets = 0;
  unsigned long timeMovingMS = 0;
  bool hudConnected = false;
  bool boardConnected = false;
  bool update = false;

  void setResetsAcknowledgedCallback(ResetsAcknowledgedCallback cb)
  {
    _resetsAcknowledgedCallback = cb;
  }

  bool wasUnintendedReset()
  {
    return soft_resets > 0;
  }

  void setResetReasons(RESET_REASON reason0, RESET_REASON reason1)
  {
    _reset_reason_core0 = reason0;
    _reset_reason_core1 = reason1;
  }

  void printResetReasons()
  {
    Serial.printf("CPU0 reset reason: %s\n", get_reset_reason_text(_reset_reason_core0));
    Serial.printf("CPU1 reset reason: %s\n", get_reset_reason_text(_reset_reason_core1));
  }

  bool watchdogReset()
  {
    return _reset_reason_core0 == RESET_REASON::SW_CPU_RESET ||
           _reset_reason_core1 == RESET_REASON::SW_CPU_RESET;
  }

  bool powerOnReset()
  {
    return _reset_reason_core0 == RESET_REASON::POWERON_RESET;
  }

  void ackResets()
  {
    // _resetsAcknowledged = true;
    soft_resets = 0;
    _resetsAcknowledgedCallback();
  }

  void addMovingTime(ulong ms)
  {
    timeMovingMS += ms;
    _storeTimeMoving();
  }

  float getTimeMovingInSeconds()
  {
    return timeMovingMS / 1000.0;
  }

  float getTimeMovingInMinutes()
  {
    return (timeMovingMS / 1000.0) / 60.0;
  }

  float getAverageAmpHoursPerSecond(float amphours)
  {
    return timeMovingMS > 0
               ? amphours / getTimeMovingInMinutes()
               : 0;
  }

  void updateHud(bool lastSentPacketResult)
  {
    if (lastSentPacketResult && hudConnected == false)
    {
      hudConnected = true;
    }
    else if (!lastSentPacketResult && hudConnected)
    {
      hudConnected = false;
    }
  }

private:
  bool _resetsAcknowledged = false;
  ulong _lastStoreTimeMs;
  ResetsAcknowledgedCallback _resetsAcknowledgedCallback;
  RESET_REASON _reset_reason_core0,
      _reset_reason_core1;

  void _storeTimeMoving()
  {
    if (timeMovingMS - _lastStoreTimeMs > STORE_SNAPSHOT_INTERVAL)
    {
      // time to store in memory
      storeInMemory<ulong>(STORE_STATS_TRIP_TIME, timeMovingMS);
      _lastStoreTimeMs = timeMovingMS;
      if (PRINT_RESET_DETECTION)
        Serial.printf("Storing time in memory now : %ums\n", timeMovingMS);
    }
  }
};