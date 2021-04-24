#pragma once

#include <Arduino.h>
#include <RTOSTaskManager.h>

class TaskBaseAlt
{
public:
  RTOSTaskManager *rtos = nullptr;

public:
  const char *_name = "Task has not name";
  bool ready = false, enabled = false;
  unsigned long doWorkInterval = PERIOD_10ms;
  bool printSendToQueue = false,
       printPeekSchedule = false;
  elapsedMillis since_last_did_work = 0;

public:
  TaskBaseAlt(const char *name, uint16_t stackSize)
  {
    _name = name;
    rtos = new RTOSTaskManager(_name, /*stack*/ stackSize);
  }
  virtual void initialiseQueues() = 0;
  virtual void initialise() = 0;
  virtual void initialTask()
  {
  } // you would have to "override"
  virtual bool timeToDoWork() = 0;
  virtual void doWork() = 0;
  virtual void start(uint8_t priority, ulong doWorkInterval, TaskFunction_t taskRef) = 0;
  virtual void deleteTask(bool print = false) = 0;

  void enable(bool print = false)
  {
    if (print)
      Serial.printf("[TASK: %s] enabled\n", _name);
    enabled = true;
  }

  void task(void *parameters)
  {
    rtos->printStarted();

    initialiseQueues();

    initialise();

    ready = true;

    while (!enabled)
      vTaskDelay(TICKS_5ms);

    initialTask();

    while (true)
    {
      if (since_last_did_work > doWorkInterval && enabled)
      {
        if (timeToDoWork())
        {
          since_last_did_work = 0;
          doWork();
        }
      }
      vTaskDelay(5);
    }
    vTaskDelete(NULL);
  }
};