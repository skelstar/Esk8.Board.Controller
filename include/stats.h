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

  void setResetsAcknowledgedCallback(ResetsAcknowledgedCallback cb)
  {
    _resetsAcknowledgedCallback = cb;
  }

  bool needToAckResets()
  {
    return soft_resets > 0 && !_resetsAcknowledged;
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
    _resetsAcknowledged = true;
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

  float getAverageAmpHoursPerSecond(float amphours)
  {
    return timeMovingMS > 0 ? amphours / getTimeMovingInSeconds() : 0;
  }

private:
  bool _resetsAcknowledged = false;
  ulong _lastStoreTimeMs;
  ResetsAcknowledgedCallback _resetsAcknowledgedCallback;
  RESET_REASON _reset_reason_core0;
  RESET_REASON _reset_reason_core1;

  void _storeTimeMoving()
  {
    if (timeMovingMS - _lastStoreTimeMs > STORE_SNAPSHOT_INTERVAL)
    {
      // time to store in memory
      storeInMemory<ulong>(STORE_STATS_TRIP_TIME, timeMovingMS);
      _lastStoreTimeMs = timeMovingMS;
#ifdef PRINT_RESET_DETECTION
      Serial.printf("Storing time in memory now : %ums\n", timeMovingMS);
#endif
    }
  }

  char *get_reset_reason_text(RESET_REASON reason)
  {
    switch (reason)
    {
    case RESET_REASON::POWERON_RESET:
      return "POWERON_RESET"; /**<1, Vbat power on reset*/
    case 3:
      return "SW_RESET"; /**<3, Software reset digital core*/
    case 4:
      return "OWDT_RESET"; /**<4, Legacy watch dog reset digital core*/
    case 5:
      return "DEEPSLEEP_RESET"; /**<5, Deep Sleep reset digital core*/
    case 6:
      return "SDIO_RESET"; /**<6, Reset by SLC module, reset digital core*/
    case 7:
      return "TG0WDT_SYS_RESET"; /**<7, Timer Group0 Watch dog reset digital core*/
    case 8:
      return "TG1WDT_SYS_RESET"; /**<8, Timer Group1 Watch dog reset digital core*/
    case 9:
      return "RTCWDT_SYS_RESET"; /**<9, RTC Watch dog Reset digital core*/
    case 10:
      return "INTRUSION_RESET"; /**<10, Instrusion tested to reset CPU*/
    case 11:
      return "TGWDT_CPU_RESET"; /**<11, Time Group reset CPU*/
    case 12:
      return "SW_CPU_RESET"; /**<12, Software reset CPU*/
    case 13:
      return "RTCWDT_CPU_RESET"; /**<13, RTC Watch dog Reset CPU*/
    case 14:
      return "EXT_CPU_RESET"; /**<14, for APP CPU, reseted by PRO CPU*/
    case 15:
      return "RTCWDT_BROWN_OUT_RESET"; /**<15, Reset when the vdd voltage is not stable*/
    case 16:
      return "RTCWDT_RTC_RESET"; /**<16, RTC Watch dog reset digital core and rtc module*/
    default:
      return "NO_MEAN";
    }
  }
};