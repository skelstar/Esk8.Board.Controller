
void display_task_0(void *pvParameters)
{
  elapsedMillis since_updated_display;

  setupLCD();

  add_disp_state_transitions();

  Serial.printf("display_task_0 running on core %d\n", xPortGetCoreID());

  int i = 0;

  while (true)
  {
    display_state.run_machine();

    DispStateEvent ev = (DispStateEvent) read_from_(xDisplayChangeEventQueue);
    if (ev == DISP_EV_REFRESH)
    {
      since_updated_display = 0;
      display_state_event(DISP_EV_REFRESH);
    }
    else if (ev != DISP_EV_NO_EVENT) {
      display_state_event(ev);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
#define BATTERY_MEASURE_PIN 34
#define BATTERY_MEASURE_INTERVAL 1000

elapsedMillis since_measure_battery;

void batteryMeasureTask_0(void *pvParameters)
{
  Serial.printf("batteryMeasureTask_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    if (since_measure_battery > BATTERY_MEASURE_INTERVAL)
    {
      since_measure_battery = 0;
      uint16_t remote_battery_volts_raw = analogRead(BATTERY_MEASURE_PIN);
      remote_battery_percent = get_remote_battery_percent(remote_battery_volts_raw);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------

void deadmanTask_0(void *pvParameters)
{
  Serial.printf("deadmanTask_0 running on core %d\n", xPortGetCoreID());

  deadman_init();

  while (true)
  {
    deadman.loop();
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
