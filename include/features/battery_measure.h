
#define BATTERY_MEASURE_INTERVAL 5000

elapsedMillis since_measure_battery;

// prototypes
void battVoltsChanged_cb();

//--------------------------------------------------------
void batteryMeasureTask_0(void *pvParameters)
{
  remote_batt.setup(battVoltsChanged_cb);

  Serial.printf("batteryMeasureTask_0 running on core %d\n", xPortGetCoreID());

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
void createBatteryMeasureTask(uint8_t core, uint8_t priority)
{
  xTaskCreatePinnedToCore(
      batteryMeasureTask_0,
      "batteryMeasureTask_0",
      10000,
      NULL,
      priority,
      NULL,
      core);
}

//--------------------------------------------------------
void battVoltsChanged_cb()
{
  displayChangeQueueManager->send(DispState::UPDATE);
}
//--------------------------------------------------------
