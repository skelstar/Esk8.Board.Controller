
#define BATTERY_MEASURE_INTERVAL 5000

elapsedMillis since_measure_battery;

// prototypes
void battVoltsChanged_cb();

//--------------------------------------------------------
void batteryMeasureTask_0(void *pvParameters)
{
  RTOSUtils::printTaskDetails();

  remote_batt.setup(battVoltsChanged_cb);

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
void battVoltsChanged_cb()
{
  displayChangeQueueManager->send(Disp::DISP_EV_UPDATE);
}
//--------------------------------------------------------
