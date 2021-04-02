#pragma once

#include <Arduino.h>
#include <elapsedMillis.h>

#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define WITH_HEALTHCHECK 1

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
    if (print)
    {
      Serial.printf("------------------------\n");
      Serial.printf("DELETING %s!\n", _task_name);
      Serial.printf("------------------------\n");
    }
    vTaskDelay(100);
    vTaskDelete(_taskHandle);
  }

  void create(
      TaskFunction_t func,
      int core,
      int priority,
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
  }

private:
  const char *_task_name;
  int _stackSize, _priority, _core;
  bool _health_check;
  TaskHandle_t _taskHandle;
  elapsedMillis
      _since_health_check,
      _since_created;
};