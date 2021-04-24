#pragma once

#include <Arduino.h>
#include <RTOSTaskManager.h>

class TaskBase
{
public:
  RTOSTaskManager *rtos = nullptr;

  typedef void (*VoidVoidCallback)();
  typedef bool (*BoolVoidCallback)();

public:
  VoidVoidCallback _initialise_cb = nullptr;
  VoidVoidCallback _initialiseQueues_cb = nullptr;
  VoidVoidCallback _createTask_cb = nullptr;
  VoidVoidCallback _firstTask_cb = nullptr;
  BoolVoidCallback _timeToDoWork_cb = nullptr;
  VoidVoidCallback _doWork_cb = nullptr;

public:
  const char *_name = "Task has not name";
  bool ready = false, enabled = false;
  unsigned long doWorkInterval = PERIOD_10ms;
  bool printSendToQueue = false,
       printPeekSchedule = false;
  elapsedMillis since_last_did_work = 0;

public:
  // TaskBase()
  // {
  // }

  TaskBase(const char *name, uint16_t stackSize)
  {
    _name = name;
    rtos = new RTOSTaskManager(_name, /*stack*/ stackSize);
  }

  void enable(bool print = false)
  {
    if (print)
      Serial.printf("[TASK: %s] enabled\n", _name);
    enabled = true;
  }

  void task(void *parameters)
  {
    rtos->printStarted();

    if (_initialiseQueues_cb != nullptr)
      _initialiseQueues_cb();

    if (_initialise_cb != nullptr)
      _initialise_cb();

    ready = true;

    while (!enabled)
      vTaskDelay(TICKS_5ms);

    if (_firstTask_cb != nullptr)
      _firstTask_cb();

    while (true)
    {
      if (since_last_did_work > doWorkInterval && enabled)
      {
        if (_timeToDoWork_cb != nullptr && _timeToDoWork_cb())
        {
          since_last_did_work = 0;
          if (_doWork_cb != nullptr)
            _doWork_cb();
        }
        else if (_timeToDoWork_cb == nullptr)
          Serial.printf("ERROR: _timeToDoWork callback is NULL (%s)\n", _name);
      }
      vTaskDelay(5);
    }
    vTaskDelete(NULL);
  }

  void deleteTask(bool print = false)
  {
    rtos->deleteTask(print);
  }

  void setInitialiseCallback(VoidVoidCallback _cb)
  {
    _initialise_cb = _cb;
  }

  void setInitialiseQueuesCallback(VoidVoidCallback _cb)
  {
    _initialiseQueues_cb = _cb;
  }

  void setCreateTaskCallback(VoidVoidCallback _cb)
  {
    _createTask_cb = _cb;
  }

  void setFirstTaskCallback(VoidVoidCallback _cb)
  {
    _firstTask_cb = _cb;
  }

  void setTimeToDoWorkCallback(BoolVoidCallback _cb)
  {
    _timeToDoWork_cb = _cb;
  }

  void setDoWorkCallback(VoidVoidCallback _cb)
  {
    _doWork_cb = _cb;
  }
};
