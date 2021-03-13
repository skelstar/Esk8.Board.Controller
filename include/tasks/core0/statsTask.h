
#include <statsClass.h>

//------------------------------------------------------------------

StatsClass stats;

template <typename T>
void storeInMemory(char *storeName, char *key, T value);
template <typename T>
T readFromMemory(char *storeName, char *key, T defaultVal = 0);

namespace Stats
{
  // prototypes
  bool handleStartingStopping(BoardClass *brd);

  bool taskReady = false;
  elapsedMillis sinceReadQueue, since_started_moving = 0;

  BoardClass *myboard;

  enum ResetsType
  {
    NO_TYPE = 0,
    CONTROLLER_RESETS,
    BOARD_RESETS,
  };

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Stats", xPortGetCoreID());

    taskReady = true;
    myboard = new BoardClass();

    ulong queueReadPeriod = 100;

    while (true)
    {
      if (sinceReadQueue > 100)
      {
        sinceReadQueue = 0;

        BoardClass *res = boardPacketQueue->peek<BoardClass>();
        if (res != nullptr)
        {
          if (myboard->packet.moving != res->packet.moving)
            if (handleStartingStopping(res))
              // stats changed
              statsQueue->send(&stats);
          myboard = new BoardClass(*res);
        }
      }

      vTaskDelay(10);
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

  void init()
  {
    // TODO SPI mutex

    // get the number of resets
    stats.controllerResets = readFromMemory<uint16_t>(STORE_STATS, STORE_STATS_SOFT_RSTS);

    stats.setResetReasons(rtc_get_reset_reason(0), rtc_get_reset_reason(1));
    stats.setResetsAcknowledgedCallback(resetsAcknowledged_callback);
  }

  bool handleStartingStopping(BoardClass *brd)
  {
    bool updated = false;
    if (brd->packet.moving)
    {
      // start the clock
      since_started_moving = 0;
    }
    else if (brd->packet.moving == false)
    {
      if (since_started_moving > 5000)
      {
        // store moving time in memory
        stats.addMovingTime(since_started_moving);
        storeInMemory<unsigned long>(STORE_STATS, STORE_STATS_TRIP_TIME, stats.timeMovingMS);
        updated = true;
      }
    }
    return updated;
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
