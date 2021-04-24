#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

#ifndef NintendoController_h
// in case a mock is being used
#include <NintendoController.h>
#endif

namespace NintendoClassicTaskBase
{
  bool printWarnings = true;

  // prototypes
  void connectToNintendoController();
  uint8_t button_changed(uint8_t *new_buttons, uint8_t *old_buttons);
  // const char *getButtonName(uint8_t button);

  TaskBase *thisTask;

  NintendoController classic;

  namespace _p
  {
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

    elapsedMillis since_last_did_work = 0;

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

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start(uint8_t priority, ulong doWorkInterval)
  {
    thisTask = new TaskBase("NintendoClassicTaskBase", 3000);
    thisTask->setInitialiseCallback(_p::initialise);
    thisTask->setInitialiseQueuesCallback(_p::initialiseQueues);
    thisTask->setTimeToDoWorkCallback(_p::timeToDoWork);
    thisTask->setDoWorkCallback(_p::doWork);

    thisTask->doWorkInterval = doWorkInterval;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(_p::task, CORE_0, priority, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }

  //--------------------------------------------------
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

  // const char *getButtonName(uint8_t button)
  // {
  //   switch (button)
  //   {
  //   case NintendoController::BUTTON_UP:
  //     return "BUTTON_UP";
  //   case NintendoController::BUTTON_RIGHT:
  //     return "BUTTON_RIGHT";
  //   case NintendoController::BUTTON_DOWN:
  //     return "BUTTON_DOWN";
  //   case NintendoController::BUTTON_LEFT:
  //     return "BUTTON_LEFT";
  //   case NintendoController::BUTTON_A:
  //     return "BUTTON_A";
  //   case NintendoController::BUTTON_B:
  //     return "BUTTON_B";
  //   case NintendoController::BUTTON_START:
  //     return "BUTTON_START";
  //   case NintendoController::BUTTON_SELECT:
  //     return "BUTTON_SELECT";
  //   case NintendoController::BUTTON_COUNT:
  //     return "BUTTON_COUNT";
  //   case NintendoController::BUTTON_NONE:
  //     return "BUTTON_NONE";
  //   }
  //   return "OUT OF RANGE: getButtonName()";
  // }
}