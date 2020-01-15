#define LINE_1 1
#define LINE_2 2
#define LINE_3 3

void screen_show_connecting()
{
  u8g2.clearBuffer();
  // line 1
  lcd_message(
      LINE_1,
      "Connecting",
      ALIGNED_LEFT);
  draw_small_battery(remote_battery_percent, 128 - SM_BATT_WIDTH, 0);

  u8g2.sendBuffer();
}

void screen_with_stats(uint8_t trigger_state, bool moving)
{
  char buff2[12];

  if (xSPISemaphore_take(100))
  {
    u8g2.clearBuffer();
    xSemaphoreGive(xSPISemaphore);
  }

  // line 1
  lcd_message(LINE_1, "Stopped", ALIGNED_LEFT);
  draw_small_battery(remote_battery_percent, 128 - SM_BATT_WIDTH, 0);
  // line 2
  sprintf(buff2, "rate: %d%%", stats.retry_rate);
  lcd_message(LINE_2, buff2);
  // line 3
  sprintf(buff2, "bd rsts: %d", board_first_packet_count);
  lcd_message(LINE_3, buff2);
  if (xSPISemaphore_take(100))
  {
    u8g2.sendBuffer();
    xSemaphoreGive(xSPISemaphore);
  }
}