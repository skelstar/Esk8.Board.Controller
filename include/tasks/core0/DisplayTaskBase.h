#include <TaskBase.h>
#include <tasks/queues/queues.h>
#include <types/SendToBoardNotf.h>
#include <types/DisplayEvent.h>

namespace DisplayTaskBase
{
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

  // prototypes

  TaskBase *thisTask;

  DisplayEvent displayEvent;

  namespace _p
  {
    Queue1::Manager<SendToBoardNotf> *scheduleQueue = nullptr;
    Queue1::Manager<PacketState> *packetStateQueue = nullptr;
    Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
    Queue1::Manager<NintendoButtonEvent> *nintendoClassicQueue = nullptr;
    Queue1::Manager<DisplayEvent> *displayEventQueue = nullptr;

    void initialiseQueues()
    {
      scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(DisplayBase)ScheduleQueue");
      packetStateQueue = Queue1::Manager<PacketState>::create("(DisplayBase)PacketStateQueue");
      primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("(DisplayBase)PrimaryButtonQueue");
      nintendoClassicQueue = Queue1::Manager<NintendoButtonEvent>::create("(DisplayBase)NintendoClassicQueue");
      displayEventQueue = Queue1::Manager<DisplayEvent>::create("(DisplayBase)DisplayEventQueue");
    }

    void initialise()
    {
      if (scheduleQueue == nullptr ||
          packetStateQueue == nullptr ||
          primaryButtonQueue == nullptr ||
          nintendoClassicQueue == nullptr ||
          displayEventQueue == nullptr)
        Serial.printf("[TASK:%s] one of the queues is NULL\n", thisTask->_name);

      if (mux_SPI == nullptr)
        mux_SPI = xSemaphoreCreateMutex();
    }

    elapsedMillis since_last_did_work = 0;

    bool timeToDowork()
    {
      return since_last_did_work > thisTask->doWorkInterval && thisTask->enabled;
    }

    void doWork()
    {
      bool OK = scheduleQueue->hasValue();

      // TODO logic for NO_CORRELATION
      if (OK && scheduleQueue->payload.command == QueueBase::RESPOND)
        thisTask->respondToOrchestrator<DisplayEvent>(scheduleQueue->payload, displayEvent, displayEventQueue);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start(unsigned long doWorkInterval)
  {
    thisTask = new TaskBase("DisplayTaskBase", 3000);
    thisTask->setInitialiseQueuesCallback(_p::initialiseQueues);
    thisTask->setInitialiseCallback(_p::initialise);
    thisTask->setTimeToDoWorkCallback(_p::timeToDowork);
    thisTask->setDoWorkCallback(_p::doWork);

    thisTask->doWorkInterval = doWorkInterval > PERIOD_10ms ? doWorkInterval : PERIOD_10ms;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(_p::task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }

  //--------------------------------------------------
}