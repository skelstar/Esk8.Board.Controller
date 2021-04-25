#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

class QwiicButtonTask : public TaskBase
{
public:
  // variables
  bool printSendToQueue = false;

  QwiicButtonTask(unsigned long p_doWorkInterval) : TaskBase("QwiicTask", 3000, p_doWorkInterval)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_2;
  }

private:
  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

  QwiicButton qwiicButton;

  PrimaryButtonState state;

  void initialiseQueues()
  {
    primaryButtonQueue = createQueue<PrimaryButtonState>("(QwiicButtonTask)primaryButtonQueue");
  }

  void initialise()
  {
    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    state.pressed = 0;

    connectToQwiicButton();
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    bool wasPressed = state.pressed;
    if (take(mux_I2C, TICKS_10ms))
    {
      state.pressed = qwiicButton.isPressed();
      give(mux_I2C);
    }

    if (wasPressed != state.pressed)
      primaryButtonQueue->send(&state, printSendToQueue ? QueueBase::printSend : nullptr);
  }

  void connectToQwiicButton()
  {
    bool initButton = false;
    do
    {
      if (take(mux_I2C, TICKS_10ms))
      {
        initButton = qwiicButton.begin();
        if (!initButton)
          Serial.printf("ERROR: couldn't init QwiicButton\n");
        give(mux_I2C);
      }
      vTaskDelay(200);
    } while (!initButton);

    Serial.printf("Qwiic Button initialised\n");
  }
};

//--------------------------------------------------

QwiicButtonTask qwiicButtonTask(PERIOD_100ms);

namespace nsQwiicButtonTask
{
  void task1(void *parameters)
  {
    qwiicButtonTask.task(parameters);
  }
}