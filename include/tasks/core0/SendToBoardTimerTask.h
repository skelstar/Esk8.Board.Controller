#include <elapsedMillis.h>
#include <RTOSTaskManager.h>

/************************************
 * Once enabled, this will send out a
 * "notification" on it's queue once
 * every SEND_TO_BOARD_INTERVAL
 * **********************************/

namespace SendToBoardTimerTask
{
  elapsedMillis
      since_sent_to_board = 0;

  namespace
  {
    unsigned long sendInterval = SEND_TO_BOARD_INTERVAL;
  }

  RTOSTaskManager mgr("SendToBoardTimerTask", 5000);

  //----------------------------------------------
  void setSendInterval(unsigned long interval, bool print = false)
  {
    sendInterval = interval;
    if (print)
      Serial.printf("SendToBoardTimerTask: Send interval has been set to %lums\n",
                    sendInterval);
  }

  //==============================================
  void task(void *pvParameters)
  {
    mgr.printStarted();

    SendToBoardNotf notification;

    Queue1::Manager<SendToBoardNotf> sendToBoardQueue(xSendToBoardQueueHandle, TICKS_5ms, "(IRL)sendNotf");

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {
      if (since_sent_to_board > sendInterval && mgr.enabled())
      {
        since_sent_to_board = 0;

        notification.sent_time = millis();

        sendToBoardQueue.send_r(&notification);
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //==============================================
} // namespace SendToBoardTimerTask
