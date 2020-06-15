#include <BatteryLib.h>

#define BATTERY_MEASURE_PIN 34
#define BATTERY_MEASURE_INTERVAL 5000

elapsedMillis since_measure_battery;
BatteryLib remote_batt(BATTERY_MEASURE_PIN);

// prototypes
void battery_value_changed_cb();

//--------------------------------------------------------
void batteryMeasureTask_0(void *pvParameters)
{
  remote_batt.setup(battery_value_changed_cb, /*increments(%)*/ 5);

  Serial.printf("batteryMeasureTask_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    if (since_measure_battery > BATTERY_MEASURE_INTERVAL)
    {
      since_measure_battery = 0;
      remote_batt.read_remote_battery();
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//--------------------------------------------------------
void battery_value_changed_cb()
{
  remote_battery_percent = remote_batt.remote_battery_percent;
  send_to_display_event_queue(DISP_EV_UPDATE);
}
//--------------------------------------------------------
