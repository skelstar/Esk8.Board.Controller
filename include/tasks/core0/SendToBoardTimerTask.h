#include <elapsedMillis.h>
#include <RTOSTaskManager.h>

namespace SendToBoardTimerTask
{
  elapsedMillis
      since_sent_to_board = 0;

  RTOSTaskManager mgr("SendToBoardTimerTask", 3000);

  //==============================================
  void task(void *pvParameters)
  {
    mgr.printStarted();

    SendToBoardNotf notification;

    notification.event_id = 0;

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {
      if (since_sent_to_board > SEND_TO_BOARD_INTERVAL && mgr.enabled())
      {
        since_sent_to_board = 0;

        sendToBoardQueue->send(&notification);
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //==============================================
} // namespace SendToBoardTimerTask
