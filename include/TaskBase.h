#pragma once

#include <Arduino.h>
#include <RTOSTaskManager.h>

class TaskBase
{
public:
  RTOSTaskManager *rtos;

public:
  TaskBase(const char *name)
  {
    rtos = new RTOSTaskManager(_name, /*stack*/ 3000);
  }

  void task(void *parameters)
  {
    rtos->printStarted();

    //remove
    bool notfQueue_hasContent = true;

    if (_initialiseQueues_cb != nullptr)
      _initialiseQueues_cb();

    if (_initialise_cb != nullptr)
      _initialise_cb();

    while (true)
    {
      if (notfQueue_hasContent)
      {
        if (_doWork_cb == nullptr)
          _doWork_cb();
      }
    }
  }

  typedef void (*VoidVoidCallback)();

  VoidVoidCallback _initialise_cb = nullptr;
  VoidVoidCallback _initialiseQueues_cb = nullptr;
  VoidVoidCallback _createTask_cb = nullptr;
  VoidVoidCallback _doWork_cb = nullptr;

private:
  const char *_name = "Task has not name";
};

class ExampleTask : public TaskBase
{
  ExampleTask(const char *name) : TaskBase(name)
  {
  }

  void task(void *parameters)
  {
  }
} exampleTask;

int main(int argc, char const *argv[])
{
  ExampleTask exampleTask("example");

  /* code */
  return 0;
}