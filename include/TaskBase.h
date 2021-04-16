#pragma once

#include <Arduino.h>
#include <RTOSTaskManager.h>
#include <QueueManager.h>

class TaskBase
{
public:
  RTOSTaskManager *rtos = nullptr;

  typedef void (*VoidVoidCallback)();
  typedef bool (*BoolVoidCallback)();

public:
  TaskBase(const char *name, uint16_t stackSize)
  {
    _name = name;
    rtos = new RTOSTaskManager(_name, /*stack*/ stackSize);
  }

  void task(void *parameters)
  {
    rtos->printStarted();

    if (_initialiseQueues_cb != nullptr)
      _initialiseQueues_cb();

    if (_initialise_cb != nullptr)
      _initialise_cb();

    while (true)
    {
      if (_timeToDoWork_cb != nullptr && _timeToDoWork_cb())
      {
        if (_doWork_cb != nullptr)
          _doWork_cb();
      }
      else if (_timeToDoWork_cb == nullptr)
        Serial.printf("ERROR: _timeToDoWork callback is NULL\n");
      vTaskDelay(5);
    }
    vTaskDelete(NULL);
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

  void setTimeToDoWorkCallback(BoolVoidCallback _cb)
  {
    _timeToDoWork_cb = _cb;
  }

  void setDoWorkCallback(VoidVoidCallback _cb)
  {
    _doWork_cb = _cb;
  }

  Queue1::Manager<SendToBoardNotf> *sendNotfQueue;
  Queue1::Manager<PrimaryButtonState> *readPrimaryButtonQueue;
  Queue1::Manager<ThrottleState> *readThrottleQueue;
  Queue1::Manager<PacketState> *readPacketStateQueue;
  Queue1::Manager<NintendoButtonEvent> *readNintendoQueue;

  // Queue1::Manager<SendToBoardNotf> *sendNotfQueue = SendToBoardTimerTask::createQueueManager("test)sendNotfQueue");
  // Queue1::Manager<PrimaryButtonState> *readPrimaryButtonQueue = QwiicButtonTask::createQueueManager("(test)readPrimaryButtonQueue");
  // Queue1::Manager<ThrottleState> *readThrottleQueue = ThrottleTask::createQueueManager("(test)readThrottleQueue");
  // Queue1::Manager<PacketState> *readPacketStateQueue = BoardCommsTask::createQueueManager("(test)readPacketStateQueue");
  // Queue1::Manager<NintendoButtonEvent> *readNintendoQueue = NintendoClassicTask::createQueueManager("(test)readNintendoQueue");

  VoidVoidCallback _initialise_cb = nullptr;
  VoidVoidCallback _initialiseQueues_cb = nullptr;
  VoidVoidCallback _createTask_cb = nullptr;
  BoolVoidCallback _timeToDoWork_cb = nullptr;
  VoidVoidCallback _doWork_cb = nullptr;

  const char *_name = "Task has not name";
};
