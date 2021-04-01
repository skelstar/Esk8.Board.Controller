#include <Arduino.h>

namespace ShortLivedTask
{

  RTOSTaskManager mgr("ShortLivedTask", 3000);

  elapsedMillis since_checked, since_pinged;

  unsigned long last_id = 0;

  //=====================================================

  void task(void *pvParameters)
  {
    mgr.printStarted();

    init();

    mgr.ready = true;

    mgr.printReady();

    while (true)
    {
      if (since_checked > 100)
      {
        since_checked = 0;

        NintendoButtonEvent *ev = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);
        if (ev != nullptr && !ev->been_peeked(last_id))
        {
          last_id = ev->id;
          if (ev->button == NintendoController::BUTTON_LEFT && ev->state == NintendoController::BUTTON_PRESSED)
          {
            mgr.deleteTask(/*print*/ true);
          }
        }
      }

      // mgr.healthCheck(3000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //============================================================
} // namespace ShortLivedTask
