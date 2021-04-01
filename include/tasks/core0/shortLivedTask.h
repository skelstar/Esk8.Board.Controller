#include <Arduino.h>

//=-----------------------------------------
/* prototypes */

//------------------------------------------
// #define configGENERATE_RUN_TIME_STATS 1
// #define configUSE_STATS_FORMATTING_FUNCTIONS 1

namespace ShortLivedTask
{
  /* prototypes */

  elapsedMillis since_checked, since_pinged;

  bool board_connected = false;
  unsigned long last_id = 0;

  // task
  TaskHandle_t taskHandle;
  TaskConfig config{
      "ShortLivedTask",
      /*size*/ 3000,
      taskHandle,
      /*ready*/ false};

  //=====================================================

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, config.name, xPortGetCoreID());

    init();

    config.taskReady = true;

    Serial.printf("%s ready\n", config.name);
    Serial.printf("Ready after %lums of being created\n", (unsigned long)since_task_created);

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
            // delete task
            Serial.printf("------------------------\n");
            Serial.printf("DELETING ShortLivedTask!\n");
            Serial.printf("------------------------\n");
            vTaskDelay(100);
            vTaskDelete(NULL);
          }
        }
      }

      if (since_pinged > 3000)
      {
        since_pinged = 0;
        Serial.printf("%s ping!\n", config.name);
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
        config.name,
        config.stackSize,
        NULL,
        priority,
        &config.taskHandle,
        core);
  }
} // namespace Debug
