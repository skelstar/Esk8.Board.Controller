
uint16_t remote_battery_percent = 0;

void display_task_0(void *pvParameters)
{
  elapsedMillis since_updated_display;

  setupLCD();

  Serial.printf("display_task_0 running on core %d\n", xPortGetCoreID());

  int i = 0;

  while (true)
  {
    if (read_from_(xDisplayChangeEventQueue) == 1 || since_updated_display > 5000)
    {
      since_updated_display = 0;

      u8g2.clearBuffer();
      // line 1
      char buff1[20];
      sprintf(buff1, "rate: %.1f%%", retry_log.get());
      lcd_message(1, buff1, Aligned::ALIGNED_LEFT);
      // line 2
      char buff2[20];
      sprintf(buff2, "total: %lu", stats.total_failed);
      lcd_message(2, buff2, Aligned::ALIGNED_LEFT);
      // line 3
      char buff3[20];
      sprintf(buff3, "w/rt: %lu", stats.num_packets_with_retries);
      lcd_message(3, buff3, Aligned::ALIGNED_LEFT);
      // battery
      draw_small_battery(remote_battery_percent, 128 - SM_BATT_WIDTH, 0);
      // deadman
      draw_trigger_state(trigger.get_current_state(), BR_DATUM);
      u8g2.sendBuffer();
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
