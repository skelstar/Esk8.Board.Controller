
#include <stats.h>

//------------------------------------------------------------------

StatsClass stats;

template <typename T>
void storeInMemory(char *storeName, char *key, T value);
template <typename T>
T readFromMemory(char *storeName, char *key, T defaultVal = 0);

namespace Stats
{
  xQueueHandle xStatsQueue;
  Queue::Manager *queue;

  bool taskReady = false;

  enum StatsEvent
  {
    NONE = 0,
    STOPPED,
    MOVING,
    CLEAR_RESETS,
  };

  char *getName(uint8_t ev)
  {
    switch (ev)
    {
    case NONE:
      return "NONE";
    case STOPPED:
      return "STOPPED";
    case MOVING:
      return "MOVING";
    case CLEAR_RESETS:
      return "CLEAR_RESETS";
    }
    return "Out of range (StatsQueue getName())";
  }

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Stats", xPortGetCoreID());

    taskReady = true;

    ulong queueReadPeriod = 100;

    elapsedMillis sinceReadQueue, sinceStartedMoving = 0;

    while (true)
    {
      if (sinceReadQueue > queueReadPeriod)
      {
        sinceReadQueue = 0;

        StatsEvent event = queue->read<StatsEvent>();

        switch (event)
        {
        case StatsEvent::STOPPED:
          if (sinceStartedMoving > 5000)
          {
            // store moving time in memory
            stats.addMovingTime(sinceStartedMoving);
            storeInMemory<unsigned long>(STORE_STATS, STORE_STATS_TRIP_TIME, stats.timeMovingMS);
          }
          break;
        case StatsEvent::MOVING:
          // start the clock
          sinceStartedMoving = 0;
          break;
        case StatsEvent::CLEAR_RESETS:
          stats.ackResets();
          break;
        }
      }

      vTaskDelay(5);
    }
    vTaskDelete(NULL);
  }
  //-----------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "statsCore",
        2000,
        NULL,
        priority,
        NULL,
        core);
  }
  //-----------------------------------------------------
  void resetsAcknowledged_callback()
  {
    storeInMemory<uint16_t>(STORE_STATS, STORE_STATS_SOFT_RSTS, 0);
  }

  void queueSentEventCb(uint16_t ev)
  {
    if (PRINT_STATS_QUEUE_SEND)
      Serial.printf(PRINT_QUEUE_SEND_FORMAT, getName(ev), "STATS");
  }
  void queueReadEventCb(uint16_t ev)
  {
    if (PRINT_STATS_QUEUE_READ)
      Serial.printf(PRINT_QUEUE_READ_FORMAT, "STATS", getName(ev));
  }

  void init()
  {
    xStatsQueue = xQueueCreate(/*len*/ 3, sizeof(uint8_t));
    queue = new Queue::Manager(/*length*/ 3, sizeof(StatsEvent), /*ticks*/ 5);
    queue->setName("queue");
    queue->setSentEventCallback(queueSentEventCb);
    queue->setReadEventCallback(queueReadEventCb);

    // get the number of resets
    stats.soft_resets = readFromMemory<uint16_t>(STORE_STATS, STORE_STATS_SOFT_RSTS);

    stats.setResetReasons(rtc_get_reset_reason(0), rtc_get_reset_reason(1));
    stats.setResetsAcknowledgedCallback(resetsAcknowledged_callback);
  }

  void storeTimeMovingInMemory()
  {
    storeInMemory<ulong>(STORE_STATS, STORE_STATS_TRIP_TIME, stats.timeMovingMS);
  }

} // namespace Stats

template <typename T>
void storeInMemory(char *storeName, char *key, T value)
{
  Preferences store;
  store.begin(storeName, /*read-only*/ false);
  if (std::is_same<T, uint16_t>::value)
  {
    store.putUInt(key, value);
  }
  else if (std::is_same<T, unsigned long>::value)
  {
    store.putULong(key, value);
  }
  else
  {
    Serial.printf("WARNING: unhandled type for storeInMemory()\n");
  }
  store.end();
}

template <typename T>
T readFromMemory(char *storeName, char *key, T defaultVal)
{
  T result;
  Preferences store;
  store.begin(storeName, /*read-only*/ false);
  if (std::is_same<T, uint16_t>::value)
  {
    result = store.getUInt(key, defaultVal);
  }
  else if (std::is_same<T, unsigned long>::value)
  {
    result = store.getULong(key, defaultVal);
  }
  else
  {
    Serial.printf("WARNING: unhandled type for storeInMemory()\n");
  }
  store.end();
  return result;
}
