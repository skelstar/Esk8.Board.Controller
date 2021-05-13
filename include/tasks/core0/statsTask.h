#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

class StatsTask : public TaskBase
{
public:
  // bool printWarnings = true;
  bool printQueueRx = false,
       printAllSuccessRate = false,
       printOnlyFailedPackets = false;

private:
  Queue1::Manager<Transaction> *transactionQueue = nullptr;

  Transaction _transaction;

  enum SendResult
  {
    NONE = 0,
    OK,
    FAIL,
  };

  static const uint16_t NUM_RESULT_SAMPLES = 20;
  uint16_t _resultsIndex = 0;
  SendResult _resultsWindow[NUM_RESULT_SAMPLES];
  uint16_t _totalFailed = 0, _totalSentOK = 0;

public:
  StatsTask() : TaskBase("StatsTask", 3000, PERIOD_100ms)
  {
    _core = CORE_0;
    _priority = TASK_PRIORITY_0;
  }

  void _initialise()
  {
    transactionQueue = createQueueManager<Transaction>("(StatsTask) transactionQueue");
  }

  void doWork()
  {
    if (transactionQueue->hasValue())
    {
      _transaction = transactionQueue->payload;

      _handleTransaction(_transaction);
    }
  }

private:
  unsigned long _lastPacketId = 0;

  void _handleTransaction(Transaction transaction)
  {
    if (transaction.packet_id <= _lastPacketId)
      // we already registered this packet
      return;

    // register this packet
    switch (transaction.sendResult)
    {
    case Transaction::SENT_OK:
    case Transaction::UPDATE:
      _totalSentOK++;
      _resultsWindow[_resultsIndex] = SendResult::OK;
      break;
    case Transaction::SEND_FAIL:
      _totalFailed++;
      _resultsWindow[_resultsIndex] = SendResult::FAIL;
      break;
    }

    _lastPacketId = transaction.packet_id;

    _resultsIndex++;
    if (_resultsIndex == NUM_RESULT_SAMPLES)
      _resultsIndex = 0;

    // print
    if (printAllSuccessRate ||
        (printOnlyFailedPackets && transaction.sendResult == Transaction::SEND_FAIL))
      Serial.printf("Success rates  total=%.2f  window=%.2f   sendResult=%s  \n",
                    _getTotalSuccessRatio(), _getWindowSuccessRatio(), transaction.getSendResult());
  }

  float _getTotalSuccessRatio()
  {
    return _totalSentOK * 1.0 / (_totalSentOK + _totalFailed * 1.0);
  }

  float _getWindowSuccessRatio()
  {
    uint16_t fails = 0, OKs = 0;
    for (result : _resultsWindow)
    {
      if (result == SendResult::NONE)
        break;
      if (result == SendResult::OK)
        OKs++;
      else if (result == SendResult::FAIL)
        fails++;
    }
    return (OKs * 1.0) / (OKs + fails * 1.0);
  }
};
//=================================================

StatsTask statsTask;

namespace nsStatsTask
{
  void task1(void *parameters)
  {
    statsTask.task(parameters);
  }
}

// #include <statsClass.h>
// #include <Preferences.h>

// #ifndef STORE_STATS
// #define STORE_STATS "stats"
// #endif
// #ifndef STORE_STATS_SOFT_RSTS
// #define STORE_STATS_SOFT_RSTS "soft resets"
// #endif
// #ifndef STORE_STATS_TRIP_TIME
// #define STORE_STATS_TRIP_TIME "trip time"
// #endif

// //------------------------------------------------------------------

// StatsClass stats;

// template <typename T>
// void storeInMemory(char *storeName, char *key, T value);
// template <typename T>
// T readFromMemory(char *storeName, char *key, T defaultVal = 0);

// namespace Stats
// {
//   // prototypes
//   bool handleStartingStopping(BoardClass *brd);

//   bool taskReady = false;
//   elapsedMillis sinceReadQueue, since_started_moving = 0;

//   BoardClass *myboard;

//   enum ResetsType
//   {
//     NO_TYPE = 0,
//     CONTROLLER_RESETS,
//     BOARD_RESETS,
//   };
//   //-----------------------------------------------------

//   void task(void *pvParameters)
//   {
//     Serial.printf(PRINT_TASK_STARTED_FORMAT, "Stats", xPortGetCoreID());

//     taskReady = true;
//     myboard = new BoardClass();

//     while (true)
//     {
//       if (sinceReadQueue > 100)
//       {
//         sinceReadQueue = 0;

//         // BoardClass *res = boardPacketQueue->peek<BoardClass>(__func__);
//         // if (res != nullptr)
//         // {
//         //   if (myboard->packet.moving != res->packet.moving)
//         //     if (handleStartingStopping(res))
//         //       // stats changed
//         //       statsQueue->sendLegacy(&stats);
//         //   myboard = new BoardClass(*res);
//         // }
//       }

//       vTaskDelay(10);
//     }
//     vTaskDelete(NULL);
//   }
//   //-----------------------------------------------------

//   void createTask(uint8_t core, uint8_t priority)
//   {
//     xTaskCreatePinnedToCore(
//         task,
//         "statsCore",
//         2000,
//         NULL,
//         priority,
//         NULL,
//         core);
//   }
//   //-----------------------------------------------------
//   void resetsAcknowledged_callback()
//   {
//     storeInMemory<uint16_t>(STORE_STATS, STORE_STATS_SOFT_RSTS, 0);
//   }

//   void init()
//   {
//     // TODO SPI mutex

//     // get the number of resets
//     stats.controllerResets = readFromMemory<uint16_t>(STORE_STATS, STORE_STATS_SOFT_RSTS);

//     stats.setResetReasons(rtc_get_reset_reason(0), rtc_get_reset_reason(1));
//     stats.setResetsAcknowledgedCallback(resetsAcknowledged_callback);
//   }

//   /* returns whether it updated stats */
//   bool handleStartingStopping(BoardClass *brd)
//   {
//     bool updated_stats = false;
//     if (brd->packet.moving)
//     {
//       // start the clock
//       since_started_moving = 0;
//     }
//     else if (brd->packet.moving == false)
//     {
//       if (since_started_moving > 5000)
//       {
//         // store moving time in memory
//         stats.addMovingTime(since_started_moving);
//         storeInMemory<unsigned long>(STORE_STATS, STORE_STATS_TRIP_TIME, stats.timeMovingMS);
//         updated_stats = true;
//       }
//     }
//     return updated_stats;
//   }
// } // namespace Stats

// template <typename T>
// void storeInMemory(char *storeName, char *key, T value)
// {
//   Preferences store;
//   store.begin(storeName, /*read-only*/ false);
//   if (std::is_same<T, uint16_t>::value)
//   {
//     store.putUInt(key, value);
//   }
//   else if (std::is_same<T, unsigned long>::value)
//   {
//     store.putULong(key, value);
//   }
//   else
//   {
//     Serial.printf("WARNING: unhandled type for storeInMemory()\n");
//   }
//   store.end();
// }

// template <typename T>
// T readFromMemory(char *storeName, char *key, T defaultVal)
// {
//   T result;
//   Preferences store;
//   store.begin(storeName, /*read-only*/ false);
//   if (std::is_same<T, uint16_t>::value)
//   {
//     result = store.getUInt(key, defaultVal);
//   }
//   else if (std::is_same<T, unsigned long>::value)
//   {
//     result = store.getULong(key, defaultVal);
//   }
//   else
//   {
//     Serial.printf("WARNING: unhandled type for storeInMemory()\n");
//   }
//   store.end();
//   return result;
// }
