#pragma once

#include <Arduino.h>
#include <elapsedMillis.h>

#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define WITH_HEALTHCHECK 1

#define CORE_0 0
#define CORE_1 1

#define TASK_PRIORITY_0 0
#define TASK_PRIORITY_1 1
#define TASK_PRIORITY_2 2
#define TASK_PRIORITY_3 3
#define TASK_PRIORITY_4 4

class RTOSTaskManager
{
public:
  bool ready = false;
  bool running = false;

  RTOSTaskManager(const char *name,
                  int stackSize)
  {
    _task_name = name;
    _stackSize = stackSize;
  }

  void printStarted()
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, _task_name, _core);
  }

  void printReady()
  {
    Serial.printf("%s ready - after %lums of being created\n", _task_name, (unsigned long)_since_created);
  }

  void healthCheck(unsigned long period)
  {
    if (_since_health_check > period)
    {
      _since_health_check = 0;
      if (_health_check)
        Serial.printf("%s ping\n", _task_name);
    }
  }

  float getStackUsage()
  {
    int highWaterMark = uxTaskGetStackHighWaterMark(_taskHandle);
    return ((highWaterMark * 1.0) / _stackSize) * 100.0;
  }

  void deleteTask(bool print = false)
  {
    if (running)
    {
      if (print)
        Serial.printf("[TASK %lums] DELETING: %s!\n", millis(), _task_name);

      vTaskDelay(100);
      vTaskDelete(_taskHandle);
    }
    else
    {
      Serial.printf("WARING: trying to delete %s but it's not running!\n", _task_name);
    }
  }

  TaskHandle_t create(
      TaskFunction_t func,
      int core = 0,
      int priority = 1,
      bool health_check = false)
  {
    _core = core;
    _since_created = 0;
    _priority = priority;
    _health_check = health_check;
    running = true;

    xTaskCreatePinnedToCore(
        func,
        _task_name,
        _stackSize,
        NULL,
        priority,
        &_taskHandle,
        _core);
    return _taskHandle;
  }

  bool enabled(bool print = false)
  {
    if (!_enabled && !_reportedNotEnabled)
    {
      Serial.printf("[TASK] %s not enabled!\n", _task_name);
      _reportedNotEnabled = true;
    }
    return _enabled;
  }

  void enable(bool print = false)
  {
    if (print)
      Serial.printf("[TASK]:%s %s\n", _task_name, "enabled");
    _reportedNotEnabled = false;
    _enabled = true;
  }

private:
  const char *_task_name;
  int _stackSize, _priority, _core;
  bool _health_check, _enabled = false, _reportedNotEnabled = false;
  TaskHandle_t _taskHandle;
  elapsedMillis
      _since_health_check,
      _since_created;
};