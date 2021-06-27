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
    primaryButtonQueue->payload.lastEvent = ev;
  }

private:
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

  // QwiicButton qwiicButton;
  Button2 primaryButton;

  void _initialise()
  {
    primaryButton.begin(PRIMARY_BUTTON_PIN);
    primaryButton.setDoubleClickHandler(nsDigitalPrimaryButtonTask::doubleClickHandler);
    primaryButton.setTripleClickHandler(nsDigitalPrimaryButtonTask::tripleClickHandler);

    primaryButtonQueue = createQueueManager<PrimaryButtonState>("(DigitalPrimaryButtonTask)primaryButtonQueue");

    primaryButtonQueue->payload.pressed = 0;
    primaryButtonQueue->payload.lastEvent = PrimaryButtonEvent::EV_NONE;
  }

  void doWork()
  {
    primaryButton.loop();

    bool wasPressed = primaryButtonQueue->payload.pressed;
    primaryButtonQueue->payload.pressed = primaryButton.isPressed();

    if (wasPressed != primaryButtonQueue->payload.pressed)
      primaryButtonQueue->sendPayload();
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