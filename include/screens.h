
void screen_not_moving(uint8_t trigger_state)
{
  char buff2[10];
  sprintf(buff2, "trig: %d", trigger_state);
  
  u8g2.clearBuffer();
  // line 1
  lcd_message(/*line#*/ 1, "Stopped", TL_DATUM);
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
  lcd_message(/*line*/ 2, buff2);
#endif
  char buff3[12];
  sprintf(buff3, "bd rsts: %d", board_first_packet_count);
  lcd_message(/*line#*/ 3, &buff3[0]);
  u8g2.sendBuffer();
}