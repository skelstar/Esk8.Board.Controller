
#define BATTERY_MEASURE_INTERVAL 5000

elapsedMillis since_measure_battery;

// prototypes

namespace Remote
{
  void battVoltsChanged_cb();

  MyMutex mutex;

  BatteryLib battery(BATTERY_MEASURE_PIN);

  namespace
  {
    const char *taskName = "";
  }

  bool taskReady = false;

  //--------------------------------------------------------
  void task(void *pvParameters)
  {
    Remote::battery.setup(battVoltsChanged_cb);

    Serial.printf(PRINT_TASK_STARTED_FORMAT, taskName, xPortGetCoreID());

    taskReady = true;

    mutex.create("remote", TICKS_2);
    mutex.enabled = true;

    while (true)
    {
      if (since_measure_battery > BATTERY_MEASURE_INTERVAL)
      {
        since_measure_battery = 0;
        if (Remote::mutex.take(__func__, TICKS_50ms))
          battery.update();

        if (battery.isCharging)
        {
          // displayQueue->send(DispState::UPDATE);
        }
        Remote::mutex.give(__func__);
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
    // displayQueue->send(DispState::UPDATE);
  }
  //--------------------------------------------------------
} // namespace Remote
