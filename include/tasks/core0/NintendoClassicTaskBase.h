#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

class NintendoClassicTaskBase : public TaskBase
{
public:
  bool printWarnings = true;

  NintendoController classic;

public:
  NintendoClassicTaskBase(unsigned long p_doWorkInterval) : TaskBase("NintendoClassicTask", 3000, p_doWorkInterval)
  {
  }

  void start(uint8_t priority, TaskFunction_t taskRef)
  {
    rtos->create(taskRef, CORE_0, priority, WITH_HEALTHCHECK);
  }

private:
  Queue1::Manager<NintendoButtonEvent> *nintendoButtonQueue = nullptr;

  NintendoButtonEvent buttonEvent;

  uint8_t last_buttons[NintendoController::BUTTON_COUNT];

  void initialiseQueues()
  {
    nintendoButtonQueue = createQueue<NintendoButtonEvent>("(NintendoClassicTaskBase)nintendoButtonQueue");
  }

  void initialise()
  {
    if (nintendoButtonQueue == nullptr)
      Serial.printf("ERROR: nintendoButtonQueue is NULL\n");

    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    connectToNintendoController();
  }

  bool timeToDoWork()
  {
    return true;
  }

  void doWork()
  {
    buttonEvent.changed = classic.update(mux_I2C, TICKS_50ms);
    buttonEvent.button = NintendoController::BUTTON_NONE;
    if (buttonEvent.changed)
    {
      uint8_t *new_buttons = classic.get_buttons();
      uint8_t button_that_changed = button_changed(new_buttons, last_buttons);
      if (button_that_changed != NintendoController::BUTTON_NONE)
      {
        // something changed, send
        buttonEvent.button = button_that_changed;
        buttonEvent.state = new_buttons[button_that_changed];

        nintendoButtonQueue->send(&buttonEvent);
        buttonEvent.printSend(nintendoButtonQueue->name);
      }
      // save
      for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
        last_buttons[i] = new_buttons[i];
    }
  }

  void cleanup()
  {
    delete (nintendoButtonQueue);
    Serial.printf("cleaning up!\n");
  }

  //================================================
private:
  void connectToNintendoController()
  {
    bool initialised = false;
    do
    {
      if (take(mux_I2C, TICKS_10ms))
      {
        initialised = classic.init(printWarnings);
        if (!initialised)
          Serial.printf("ERROR: couldn't init Nintendo Controller\n");
        give(mux_I2C);
      }
      vTaskDelay(200);
    } while (!initialised);
    Serial.printf("Nintendo Controller initialised\n");
  }

  uint8_t button_changed(uint8_t *new_buttons, uint8_t *old_buttons)
  {
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      if (old_buttons[i] != new_buttons[i])
        return i;
    return 99;
  }
};

NintendoClassicTaskBase nintendoClassTask(PERIOD_500ms);

namespace nsNintendoClassicTask
{
  void task1(void *parameters)
  {
    nintendoClassTask.task(parameters);
  }
}