#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <Button2.h>

#define DIGITALPRIMARYBUTTON_TASK

namespace nsDigitalPrimaryButtonTask
{
  // prototypes
  void doubleClickHandler(Button2 &btn);
  void tripleClickHandler(Button2 &btn);
}

class DigitalPrimaryButtonTask : public TaskBase
{
public:
  // variables
  bool printSendToQueue = false;

  DigitalPrimaryButtonTask() : TaskBase("DigitalPrimaryButtonTask", 3000)
  {
    _core = CORE_0;
  }

  void setLastEvent(PrimaryButtonEvent ev)
  {
    state.lastEvent = ev;
  }

private:
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

  // QwiicButton qwiicButton;
  Button2 primaryButton;

  PrimaryButtonState state;

  void _initialise()
  {
    primaryButton.begin(PRIMARY_BUTTON_PIN);
    primaryButton.setDoubleClickHandler(nsDigitalPrimaryButtonTask::doubleClickHandler);
    primaryButton.setTripleClickHandler(nsDigitalPrimaryButtonTask::tripleClickHandler);

    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(DigitalPrimaryButtonTask)primaryButtonQueue");

    state.pressed = 0;
    state.lastEvent = PrimaryButtonEvent::EV_NONE;
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

  void doubleClickHandler(Button2 &btn)
  {
    digitalPrimaryButtonTask.setLastEvent(PrimaryButtonEvent::EV_DOUBLE_CLICK);
  }

  void tripleClickHandler(Button2 &btn)
  {
    digitalPrimaryButtonTask.setLastEvent(PrimaryButtonEvent::EV_DOUBLE_CLICK);
  }
}