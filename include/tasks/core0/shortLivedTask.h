#include <Arduino.h>

//=-----------------------------------------
/* prototypes */

//------------------------------------------
// #define configGENERATE_RUN_TIME_STATS 1
// #define configUSE_STATS_FORMATTING_FUNCTIONS 1

namespace ShortLivedTask
{
  /* prototypes */

  const char *taskName = "ShortLivedTask";

  elapsedMillis since_checked;

  bool board_connected = false;
  unsigned long last_id = 0;

  // task
  TaskHandle_t taskHandle;
  TaskConfig config{/*size*/ 3000, taskHandle, /*ready*/ false};

  //=====================================================

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, taskName, xPortGetCoreID());

    init();

    config.taskReady = true;

    Serial.printf("%s ready\n", taskName);

    while (true)
    {
      if (since_checked > 3000)
      {
        since_checked = 0;
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
        taskName,
        config.stackSize,
        NULL,
        priority,
        &config.taskHandle,
        core);
  }
} // namespace Debug
