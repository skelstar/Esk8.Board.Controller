#define LINE_1    1
#define LINE_2    2
#define LINE_3    3


portMUX_TYPE mmux = portMUX_INITIALIZER_UNLOCKED;

void screen_show_connecting()
{
  taskENTER_CRITICAL(&mmux);

  u8g2.clearBuffer();
  // line 1
  lcd_message(
    LINE_1, 
    "Connecting", 
    ALIGNED_LEFT);
  draw_small_battery(remote_battery_percent, 128-SM_BATT_WIDTH, 0);

  u8g2.sendBuffer();

  taskEXIT_CRITICAL(&mmux);
}

void screen_with_stats(uint8_t trigger_state, bool moving)
{
  char buff2[10];
  sprintf(buff2, "trig: %d", trigger_state);
  
  taskENTER_CRITICAL(&mmux);

  u8g2.clearBuffer();
  // line 1
  lcd_message(
    LINE_1, 
    moving ? "Moving" : "Stopped", 
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
  char buff3[12];
  sprintf(buff3, "bd rsts: %d", board_first_packet_count);
  lcd_message(LINE_3, &buff3[0]);
  u8g2.sendBuffer();

  taskEXIT_CRITICAL(&mmux);
}