#pragma once

#include <Arduino.h>
#include <RTOSTaskManager.h>

class TaskBase
{
protected:
  RTOSTaskManager *rtos = nullptr;
  bool exitTask = false;
  const char *_name = "Task has not name";

private:
  unsigned long doWorkInterval = PERIOD_100ms;
  elapsedMillis since_last_did_work = 0;

public:
  bool ready = false, enabled = false;
  bool printSendToQueue = false,
       printPeekSchedule = false;

protected:
  uint8_t _core = 0,
          _priority = 0;
  TaskFunction_t taskRef = nullptr;

protected:
  TaskBase(const char *name, uint16_t stackSize, unsigned long p_doWorkInterval)
  {
    _name = name;
    doWorkInterval = p_doWorkInterval;
    rtos = new RTOSTaskManager(_name, /*stack*/ stackSize);
  }
  virtual void initialiseQueues(){};
  virtual void initialise() = 0;
  virtual void initialTask() {}
  virtual bool timeToDoWork() { return true; };
  virtual void doWork() = 0;
  virtual void cleanup(){};

public:
  virtual void start(TaskFunction_t taskRef)
  {
    rtos->create(taskRef, _core, _priority, WITH_HEALTHCHECK);
  };

  virtual void deleteTask(bool print = false)
  {
    exitTask = true;
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