
//=-----------------------------------------
/* prototypes */

//------------------------------------------
// #define configGENERATE_RUN_TIME_STATS 1
// #define configUSE_STATS_FORMATTING_FUNCTIONS 1

namespace Debug
{
  /* prototypes */

  bool taskReady = false;

  elapsedMillis since_checked;

  bool board_connected = false;
  unsigned long last_id = 0;

  //=====================================================

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Debug Task", xPortGetCoreID());

    init();

    taskReady = true;

    DEBUG("DebugTask ready!");

    while (true)
    {
      if (since_checked > 3000)
      {
        since_checked = 0;

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
