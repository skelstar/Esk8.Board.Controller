#pragma once

#define NINTENDOCLASSIC_TASK

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

class NintendoClassicTask : public TaskBase
{
public:
  bool printWarnings = true;

  NintendoController classic;

public:
  NintendoClassicTask() : TaskBase("NintendoClassicTask", 3000, PERIOD_50ms)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_1;
  }

private:
  Queue1::Manager<NintendoButtonEvent> *nintendoButtonQueue = nullptr;

  NintendoButtonEvent buttonEvent;

  uint8_t last_buttons[NintendoController::BUTTON_COUNT];

  void initialiseQueues()
  {
    nintendoButtonQueue = createQueueManager<NintendoButtonEvent>("(NintendoClassicTask)nintendoButtonQueue");
  }

  void _initialise()
  {
    if (nintendoButtonQueue == nullptr)
      Serial.printf("ERROR: nintendoButtonQueue is NULL\n");

    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    connectToNintendoController();
  }

  void doWork()
  {
    buttonEvent.changed = classic.update(mux_I2C, TICKS_50ms);
    buttonEvent.button = NintendoController::BUTTON_NONE;
    if (buttonEvent.changed)
    {
      uint8_t *new_buttons = classic.get_buttons();
      uint8_t buttonTheWasPressed = buttonPressed(new_buttons, last_buttons);
      if (buttonTheWasPressed != NintendoController::BUTTON_NONE)
      {
        // something changed, send
        buttonEvent.button = buttonTheWasPressed;

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
  }

  //================================================
private:
  void connectToNintendoController()
  {
    bool initialised = false;
    do
    {
      initialised = classic.init(mux_I2C, TICKS_500ms, printWarnings);
      if (!initialised)
        Serial.printf("ERROR: couldn't init Nintendo Controller\n");
      vTaskDelay(200);
    } while (!initialised);
    Serial.printf("Nintendo Controller initialised\n");
  }

  uint8_t buttonPressed(uint8_t *new_buttons, uint8_t *old_buttons)
  {
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      if (old_buttons[i] != new_buttons[i] && old_buttons[i] == 0)
        return i;
    return 99;
  }
};

NintendoClassicTask nintendoClassTask;

namespace nsNintendoClassicTask
{
  void task1(void *parameters)
  {
    nintendoClassTask.task(parameters);
  }
}