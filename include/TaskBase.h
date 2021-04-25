#pragma once

#include <Arduino.h>
#include <RTOSTaskManager.h>

class TaskBase
{
public:
  RTOSTaskManager *rtos = nullptr;
  bool exitTask = false;

public:
  const char *_name = "Task has not name";
  bool ready = false, enabled = false;
  unsigned long doWorkInterval = PERIOD_100ms;
  bool printSendToQueue = false,
       printPeekSchedule = false;
  elapsedMillis since_last_did_work = 0;

public:
  TaskBase(const char *name, uint16_t stackSize, unsigned long p_doWorkInterval)
  {
    _name = name;
    doWorkInterval = p_doWorkInterval;
    rtos = new RTOSTaskManager(_name, /*stack*/ stackSize);
  }
  virtual void initialiseQueues() = 0;
  virtual void initialise() = 0;
  virtual void initialTask()
  {
  } // you would have to "override"
  virtual bool timeToDoWork() = 0;
  virtual void doWork() = 0;
  virtual void start(uint8_t priority, TaskFunction_t taskRef) = 0;
  virtual void deleteTask(bool print = false)
  {
    exitTask = true;
  }
  virtual void cleanup()
  {
  }

  void enable(bool print = false)
  {
    if (print)
      Serial.printf("[TASK: %s] enabled\n", _name);
    enabled = true;
  }

  void task(void *parameters)
  {
    exitTask = false;

    rtos->printStarted();

    initialiseQueues();

    initialise();

    ready = true;

    while (!enabled)
      vTaskDelay(TICKS_5ms);

    initialTask();

    while (exitTask == false)
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

    cleanup();

    Serial.printf("[TASK] Exiting: %s\n", _name);
    Serial.printf("- highwater mark (words): %d\n", uxTaskGetStackHighWaterMark(NULL));
    Serial.printf("- free heap size (bytes): %d\n", xPortGetFreeHeapSize());

    rtos->deleteTask(PRINT_THIS);

    vTaskDelay(TICKS_100ms);

    vTaskDelete(NULL);
  }
};