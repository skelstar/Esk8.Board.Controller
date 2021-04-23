#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

// incase we aren't using a mock in a test
#ifndef __SparkFun_Qwiic_Button_H__
#include <SparkFun_Qwiic_Button.h>
#endif

namespace QwiicTaskBase
{
  // prototypes
  void connectToQwiicButton();
  bool printSendToQueue = false;

  TaskBase *thisTask;

  namespace
  {
    Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

    QwiicButton qwiicButton;

    PrimaryButtonState state;

    bool printReplyToSchedule = false;

    void initialiseQueues()
    {
      primaryButtonQueue = createQueue<PrimaryButtonState>("(QwiicTaskBase)primaryButtonQueue");
    }

    void initialise()
    {
      if (primaryButtonQueue == nullptr)
        Serial.printf("ERROR: primaryButtonQueue is NULL\n");

      if (mux_I2C == nullptr)
        mux_I2C = xSemaphoreCreateMutex();

      connectToQwiicButton();
    }

    elapsedMillis since_last_did_work = 0;

    bool timeToDowork()
    {
      return true;
    }

    void doWork()
    {
      since_last_did_work = 0;
      if (take(mux_I2C, TICKS_10ms))
      {
        state.pressed = qwiicButton.isPressed();
        give(mux_I2C);
      }
      primaryButtonQueue->send(&state, printSendToQueue ? QueueBase::printSend : nullptr);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start(uint8_t priority, ulong doWorkInterval)
  {
    thisTask = new TaskBase("QwiicTaskBase", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);

    thisTask->doWorkInterval = doWorkInterval;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, priority, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }

  //--------------------------------------------------
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
}