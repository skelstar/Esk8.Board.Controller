#include <TaskBase.h>
#include <QueueManager1.h>
#include <types/SendToBoardNotf.h>

// incase we aren't using a mock in a test
#ifndef __SparkFun_Qwiic_Button_H__
#include <SparkFun_Qwiic_Button.h>
#endif

bool take(SemaphoreHandle_t m_handle, TickType_t ticks = TICKS_5ms)
{
  if (m_handle != nullptr)
    return (xSemaphoreTake(m_handle, (TickType_t)5) == pdPASS);
  return false;
}

void give(SemaphoreHandle_t m_handle)
{
  if (m_handle != nullptr)
    xSemaphoreGive(m_handle);
}

namespace QwiicTaskBase
{
  // prototypes
  void connectToQwiicButton();

  TaskBase *thisTask;

  namespace
  {
    Queue1::Manager<SendToBoardNotf> *scheduleQueue = nullptr;
    Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;

    QwiicButton qwiicButton;

    PrimaryButtonState state;

    bool printReplyToSchedule = false;

    void initialiseQueues()
    {
      scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(QwiicTaskBase)scheduleQueue");
      primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("(QwiicTaskBase)primaryButtonQueue");
    }

    void initialise()
    {
      state.correlationId = -1;

      if (primaryButtonQueue == nullptr)
        Serial.printf("ERROR: primaryButtonQueue is NULL\n");

      if (mux_I2C == nullptr)
        mux_I2C = xSemaphoreCreateMutex();

      connectToQwiicButton();
    }

    bool timeToDowork()
    {
      uint8_t status = waitForNew(scheduleQueue, PERIOD_50ms, QueueBase::printRead);
      if (status == Response::OK)
      {
        state.correlationId = scheduleQueue->payload.correlationId;
        state.sent_time = scheduleQueue->payload.sent_time;
        return true;
      }
      return false;
    }

    void doWork()
    {
      state.pressed = qwiicButton.isPressed();
      primaryButtonQueue->reply(&state, printReplyToSchedule ? QueueBase::printSend : nullptr);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start()
  {
    thisTask = new TaskBase("QwiicTaskBase", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);
    thisTask->enabled = true;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
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
      if (take(mux_I2C, TICKS_100ms))
      {
        initButton = qwiicButton.begin();
        if (!initButton)
          Serial.printf("ERROR: couldn't init QwiicButton\n");
        give(mux_I2C);
      }
    } while (initButton = false);
    Serial.printf("Qwiic Button initialised\n");
  }
}