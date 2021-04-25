#pragma once

#include <TaskBaseAlt.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

class QwiicButtonTask : public TaskBaseAlt
{
public:
  // variables
  bool printSendToQueue = false;
  // prototypes
  // void connectToQwiicButton();

  QwiicButtonTask() : TaskBaseAlt("QwiicTask", 3000)
  {
  }

  void start(uint8_t priority, ulong p_doWorkInterval, TaskFunction_t taskRef)
  {
    doWorkInterval = p_doWorkInterval;

    rtos->create(taskRef, CORE_0, priority, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (rtos != nullptr)
      rtos->deleteTask(print);
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

QwiicButtonTask qwiicButtonTask;

namespace nsQwiicButtonTask
{
  void task1(void *parameters)
  {
    qwiicButtonTask.task(parameters);
  }
}