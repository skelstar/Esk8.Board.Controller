
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
void battVoltsChanged_cb()
{
  send_to_display_event_queue(DISP_EV_UPDATE);
}
//--------------------------------------------------------
