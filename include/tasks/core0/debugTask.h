
//=-----------------------------------------
/* prototypes */

//------------------------------------------
// #define configGENERATE_RUN_TIME_STATS 1
// #define configUSE_STATS_FORMATTING_FUNCTIONS 1

namespace Debug
{
  /* prototypes */

  bool taskReady = false;

  elapsedMillis since_checked,
      since_ping;

  bool board_connected = false;
  unsigned long last_id = -1, qwiic_id = -1;

  //=====================================================

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Debug Task", xPortGetCoreID());

    init();

    taskReady = true;

    while (NintendoClassicTask::queue == nullptr &&
           QwiicButtonTask::queue == nullptr)
    {
      vTaskDelay(500);
    }

    DEBUG("DebugTask ready!");

    while (true)
    {
      if (since_checked > 100)
      {
        since_checked = 0;

        NintendoButtonEvent *ev = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);
        if (ev != nullptr && !ev->been_peeked(last_id))
        {
          last_id = ev->id;
          if (ev->button == NintendoController::BUTTON_RIGHT && ev->state == NintendoController::BUTTON_PRESSED)
          {
            ShortLivedTask::mgr.create(ShortLivedTask::task, CORE_0);

            vTaskDelay(100);
          }
          else if (ev->button == NintendoController::BUTTON_UP && ev->state == NintendoController::BUTTON_PRESSED)
          {
            NintendoClassicTask::mgr.deleteTask(true);
          }
        }

        QwiicButtonState *qwiic = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);
        if (qwiic != nullptr && !qwiic->been_peeked(qwiic_id))
        {
          qwiic_id = qwiic->id;
          if (qwiic->pressed == true)
          {
            NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0);
          }
        }

        if (since_ping > 3000)
        {
          since_ping = 0;
          Serial.printf("DebugTask ping\n");
        }

        // Serial.printf("Qwiic stack: %.1f%% (of %d), heap: %d\n",
        //               QwiicButtonTask::getStackUsage(),
        //               QwiicButtonTask::stackSize,
        //               QwiicButtonTask::getHeapBytes());
        // Serial.printf("Nintendo stack: %.1f%% (of %d)\n",
        //               NintendoClassicTask::getStackUsage(),
        //               NintendoClassicTask::stackSize);
        // Serial.printf("ThrottleTask stack: %.1f%% (of %d)\n",
        //               ThrottleTask::getStackUsage(),
        //               ThrottleTask::stackSize);
        // Serial.println("-------------------------------------");

        // char buff[1024];
        // vTaskGetRunTimeStats(buff);
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  //============================================================

  void init()
  {
  }

  //------------------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "debugTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace Debug
