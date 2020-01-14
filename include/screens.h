#define LINE_1    1
#define LINE_2    2
#define LINE_3    3


void screen_show_connecting()
{
  u8g2.clearBuffer();
  // line 1
  lcd_message(
    LINE_1, 
    "Connecting", 
    ALIGNED_LEFT);
  draw_small_battery(remote_battery_percent, 128-SM_BATT_WIDTH, 0);

  u8g2.sendBuffer();
}

void screen_with_stats(uint8_t trigger_state, bool moving)
{
  char buff2[12];
  
  u8g2.clearBuffer();
  // line 1
  lcd_message(
    LINE_1, 
    "Stopped", 
    ALIGNED_LEFT);
  draw_small_battery(remote_battery_percent, 128-SM_BATT_WIDTH, 0);
  //line 2
#ifdef USE_DEADMAN_SWITCH  
  switch (trigger_state)
  {
    case 0: lcd_message(/*line*/ 2, "trig: go!"); break;
    case 1: lcd_message(/*line*/ 2, "trig: wait"); break;
    case 2: lcd_message(/*line*/ 2, "trig: hold"); break;
  }
#else
  sprintf(buff2, "ltcy: %ds", metrics.response_time);
  lcd_message(LINE_2, buff2);
#endif
  sprintf(buff2, "bd rsts: %d", board_first_packet_count);
  lcd_message(LINE_3, buff2);
  u8g2.sendBuffer();
}