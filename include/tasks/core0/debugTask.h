
//=-----------------------------------------
/* prototypes */

//------------------------------------------

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

        Serial.printf("Debug h/w mark: %d (words)\n", uxTaskGetStackHighWaterMark(NULL));
        Serial.printf("Qwiic h/w mark: %d (words)\n", uxTaskGetStackHighWaterMark(QwiicButtonTask::taskHandle));
        Serial.printf("Qwiic stack: %.1f%% (of %d)\n",
                      getStackCapacity(QwiicButtonTask::taskHandle, QwiicButtonTask::stackSize),
                      QwiicButtonTask::stackSize);
        Serial.printf("Nintendo stack: %.1f%% (of %d)\n",
                      getStackCapacity(NintendoClassicTask::taskHandle, NintendoClassicTask::stackSize),
                      NintendoClassicTask::stackSize);

        Serial.printf("Qwiic heap: %d (bytes)\n", xPortGetFreeHeapSize());
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
