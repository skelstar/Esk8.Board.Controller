#ifndef ARDUINO_H
#include <Arduino.h>
#include <rom/rtc.h> // for reset reason
#endif

#pragma once

// #include <utils.h>
#include <shared-utils.h>

class StatsClass
{
  typedef void (*ResetsAcknowledgedCallback)();

public:
  uint16_t total_failed_sending;
  unsigned long consecutive_resps;
  uint16_t controllerResets = 0,
           boardResets = 0,
           session_count = 0;
  unsigned long timeMovingMS = 0;
  bool boardConnected = false,
       boardConnectedThisSession = false;

  void setResetsAcknowledgedCallback(ResetsAcknowledgedCallback cb)
  {
    _controllerResetsAckdCallback = cb;
  }

  bool wasUnintendedControllerReset()
  {
    return controllerResets > 0;
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

  bool wasWatchdogReset()
  {
    return _reset_reason_core0 == RESET_REASON::SW_CPU_RESET ||
           _reset_reason_core1 == RESET_REASON::SW_CPU_RESET;
  }

  bool powerOnReset()
  {
    return _reset_reason_core0 == RESET_REASON::POWERON_RESET;
  }

  void clearControllerResets()
  {
    controllerResets = 0;
    if (_controllerResetsAckdCallback != nullptr)
      _controllerResetsAckdCallback();
  }

  void clearBoardResets()
  {
    boardResets = 0;
    if (_boardResetsAckdCallback != nullptr)
      _boardResetsAckdCallback();
  }

  void addMovingTime(ulong ms)
  {
    timeMovingMS += ms;
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

private:
  bool _resetsAcknowledged = false;
  ulong _lastStoreTimeMs;
  ResetsAcknowledgedCallback
      _controllerResetsAckdCallback = nullptr,
      _boardResetsAckdCallback = nullptr;
  RESET_REASON _reset_reason_core0,
      _reset_reason_core1;
};