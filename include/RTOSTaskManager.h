#pragma once

#include <Arduino.h>
#include <elapsedMillis.h>

class RTOSTaskManager
{
public:
  bool ready = false;

  RTOSTaskManager(const char *name, int stackSize, int priority)
  {
    _task_name = name;
    _stackSize = stackSize;
    _priority = priority;
  }

  void printStarted()
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, _task_name, _core);
  }

  void printReady()
  {
    Serial.printf("%s ready\n", _task_name);
    Serial.printf("%s ready after %lums of being created\n", _task_name, (unsigned long)_since_created);
  }

  void healthCheck(unsigned long period)
  {
    if (_since_health_check > period)
    {
      _since_health_check = 0;
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

  void create(TaskFunction_t func, int core)
  {
    _core = core;
    _since_created = 0;
    xTaskCreatePinnedToCore(
        func,
        _task_name,
        _stackSize,
        NULL,
        _priority,
        &_taskHandle,
        _core);
  }

private:
  const char *_task_name;
  int _stackSize;
  int _priority;
  int _core;
  TaskHandle_t _taskHandle;
  elapsedMillis
      _since_health_check,
      _since_created;
};