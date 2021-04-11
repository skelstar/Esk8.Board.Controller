
namespace Debug
{
  RTOSTaskManager mgr("DebugTask", 3000);

  elapsedMillis since_checked,
      since_ping;

  bool board_connected = false;
  unsigned long last_id = -1, qwiic_id = -1;

  //=====================================================

  void task(void *pvParameters)
  {
    mgr.printStarted();

    init();

    while (NintendoClassicTask::queue == nullptr &&
           primaryButtonQueue == nullptr)
    {
      vTaskDelay(500);
    }

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
          last_id = ev->event_id;
          if (ev->button == NintendoController::BUTTON_RIGHT && ev->state == NintendoController::BUTTON_PRESSED)
          {
            // ShortLivedTask::mgr.create(ShortLivedTask::task, CORE_0);

            vTaskDelay(100);
          }
          else if (ev->button == NintendoController::BUTTON_UP && ev->state == NintendoController::BUTTON_PRESSED)
          {
            NintendoClassicTask::mgr.deleteTask(true);
          }
        }

        PrimaryButtonState *qwiic = QwiicButtonTask::queue->peek<PrimaryButtonState>(__func__);
        if (qwiic != nullptr && !qwiic->been_peeked(qwiic_id))
        {
          qwiic_id = qwiic->event_id;
          if (qwiic->pressed == true)
          {
            NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, TASK_PRIORITY_1);
          }
        }
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  //============================================================

  void init()
  {
  }
} // namespace Debug
