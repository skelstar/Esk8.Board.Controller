#pragma once

#include <Arduino.h>
#include <elapsedMillis.h>

#ifndef PRINT_MUTEX_TAKE_FAIL
#define PRINT_MUTEX_TAKE_FAIL 0
#endif
#ifndef PRINT_MUTEX_TAKE_SUCCESS
#define PRINT_MUTEX_TAKE_SUCCESS 0
#endif
#ifndef PRINT_MUTEX_GIVE_SUCCESS
#define PRINT_MUTEX_GIVE_SUCCESS 0
#endif

#define REPORT_TAKEN_PERIOD true

#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"

struct TaskConfig
{
  const char *name;
  int stackSize;
  TaskHandle_t taskHandle;
  bool taskReady;
};

class MyMutex
{
public:
  bool created = false;

public:
  void create(const char *name, TickType_t defaultticks)
  {
    _name = name;
    _defaultticks = defaultticks;
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == NULL)
      Serial.printf("ERROR: unsufficient space in heap for '%s'\n", name);
    created = true;
  }

  bool enabled = true;
  elapsedMillis since_taken;

  bool take(const char *funcname, TickType_t ticks)
  {
    if (_taken)
    {
      // already taken
      if (PRINT_MUTEX_TAKE_FAIL && funcname != nullptr)
        Serial.printf("ERROR: %s tried taking %s but was already taken by %s (ticks: %d)\n",
                      funcname,
                      _name,
                      _taken_by != nullptr ? _taken_by : "anon",
                      ticks);
      else if (PRINT_MUTEX_TAKE_SUCCESS && funcname != nullptr)
        Serial.printf("MUTEX: %s took %s OK\n",
                      funcname,
                      _name);
      return false;
    }

    _taken = xSemaphoreTake(_mutex, ticks) == pdPASS || !enabled;

    if (_taken)
    {
      since_taken = 0;
      _taken_by = funcname;
    }

    if (!enabled)
      Serial.printf("WARNING: %s not enabled!\n", _name);
    return _taken;
  }

  bool take(const char *funcname = nullptr)
  {
    return take(funcname, _defaultticks);
  }

  // report: how long the mutex was used for in ms
  void give(const char *funcname = "", bool report = false)
  {
    if (_mutex == nullptr)
    {
      Serial.printf("ERROR: mutex is nullptr");
      return;
    }

    xSemaphoreGive(_mutex);

    if (report)
      Serial.printf("Given: taken %lums ago (%s)\n", (unsigned long)since_taken, funcname);

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
