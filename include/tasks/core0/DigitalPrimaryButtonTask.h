#pragma once

#define DIGITALPRIMARYBUTTON_TASK

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <Button2.h>

class DigitalPrimaryButtonTask : public TaskBase
{
public:
  // variables
  bool printSendToQueue = false;

  DigitalPrimaryButtonTask() : TaskBase("DigitalPrimaryButtonTask", 3000, PERIOD_100ms)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_2;
  }

private:
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

  // QwiicButton qwiicButton;
  Button2 primaryButton;

  PrimaryButtonState state;

  void initialise()
  {
    primaryButton.begin(PRIMARY_BUTTON_PIN);

    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(DigitalPrimaryButtonTask)primaryButtonQueue");

    state.pressed = 0;
  }

  void doWork()
  {
    primaryButton.loop();

    bool wasPressed = state.pressed;
    state.pressed = primaryButton.isPressed();

    if (wasPressed != state.pressed)
      primaryButtonQueue->send(&state, printSendToQueue ? QueueBase::printSend : nullptr);
  }

  void cleanup()
  {
    delete (primaryButtonQueue);
  }
};

//--------------------------------------------------

DigitalPrimaryButtonTask digitalPrimaryButtonTask;

namespace nsDigitalPrimaryButtonTask
{
  void task1(void *parameters)
  {
    digitalPrimaryButtonTask.task(parameters);
  }
}