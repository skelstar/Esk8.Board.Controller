#include <Arduino.h>

class QueueBase
{
public:
  unsigned long id;

  bool been_peeked(unsigned long prev_id)
  {
    return id == prev_id;
  }
};

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
    if (_taken)
    {
      // already taken
      if (funcname != nullptr)
        Serial.printf("ERROR: %s tried taking %s but was already taken by %s\n",
                      funcname, _name, _taken_by != nullptr ? _taken_by : "anon");
      return false;
    }
    _taken = xSemaphoreTake(_mutex, ticks) == pdPASS || !enabled;

    if (_taken)
      _taken_by = funcname;

    if (!enabled)
      Serial.printf("WARNING: %s not enabled!\n", _name);
    else if (!_taken && PRINT_STATS_MUTEX_TAKE_STATE)
    {
      TaskHandle_t holding_task = xSemaphoreGetMutexHolder(_mutex);
      char *task_name = pcTaskGetTaskName(holding_task);
      Serial.printf("Unable to take mutex: '%s' (%d ticks: %s task: %s)\n",
                    _name,
                    ticks,
                    funcname,
                    task_name);
    }
    return _taken;
  }

  bool take(const char *funcname = nullptr)
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

    xSemaphoreGive(_mutex);
    _taken = false;
    _taken_by = nullptr;
  }

  SemaphoreHandle_t handle()
  {
    return _mutex;
  }

private:
  bool _taken = false;
  const char *_name, *_taken_by = nullptr;
  TickType_t _defaultticks;
  SemaphoreHandle_t _mutex = nullptr;
};
