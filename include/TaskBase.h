#pragma once

#include <Arduino.h>
#include <RTOSTaskManager.h>

class TaskBase
{
public:
  RTOSTaskManager *rtos;

public:
  virtual void task(void *parameters);
  virtual void initialiseQueues();
  virtual void createTask();

  TaskBase(const char *name)
  {
  }

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