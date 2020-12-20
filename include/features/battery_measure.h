
#define BATTERY_MEASURE_INTERVAL 5000

elapsedMillis since_measure_battery;

// prototypes

namespace Battery
{
  void battVoltsChanged_cb();

  namespace
  {
    const char *taskName = "";
  }
  //--------------------------------------------------------
  void task(void *pvParameters)
  {
    remote_batt.setup(battVoltsChanged_cb);

    Serial.printf("TASK: %s on Core %d\n", taskName, xPortGetCoreID());

    while (true)
    {
      if (since_measure_battery > BATTERY_MEASURE_INTERVAL)
      {
        since_measure_battery = 0;
        remote_batt.update();
      }
      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  //--------------------------------------------------------
  void createTask(uint8_t core, uint8_t priority)
  {
    taskName = "Battery Measure";
    xTaskCreatePinnedToCore(
        task,
        taskName,
        10000,
        NULL,
        priority,
        NULL,
        core);
  }

  //--------------------------------------------------------
  void battVoltsChanged_cb()
  {
    displayQueue->send(DispState::UPDATE);
  }
  //--------------------------------------------------------
} // namespace Battery