#include <Arduino.h>

class MyMutex
{
public:
  void create(const char *name, TickType_t defaultticks)
  {
    _name = name;
    _defaultticks = defaultticks;
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == NULL)
      Serial.printf("ERROR: unsufficient space in heap for '%s'\n", name);
  }

  bool enabled = true;

  bool take(const char *funcname, TickType_t ticks)
  {
    bool success = xSemaphoreTake(_mutex, ticks) == pdPASS || !enabled;
    if (!success && PRINT_STATS_MUTEX_TAKE_STATE)
      Serial.printf("Unable to take mutex: '%s' (%d ticks: %s)\n",
                    _name, ticks, funcname);
    return success;
  }

  bool take(const char *funcname)
  {
    return take(funcname, _defaultticks);
  }

  void give(const char *funcname = "")
  {
    if (_mutex == nullptr)
    {
      Serial.printf("ERROR: mutex is nullptr");
      return;
    }
    if (xSemaphoreGive(_mutex) != pdPASS && PRINT_STATS_MUTEX_GIVE_STATE)
      Serial.printf("WARNING: unable to give mutex: '%s' (%s)\n", _name, funcname);
  }

private:
  const char *_name;
  TickType_t _defaultticks;
  SemaphoreHandle_t _mutex = nullptr;
};
