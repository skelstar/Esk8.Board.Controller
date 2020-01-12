

void draw_missing_packets_screen(bool force = false)
{
  // bool something_changed = vescdata.ampHours != old_vescdata.ampHours ||
  //   board_first_packet_count != old_board_first_packet_count;

  // if (something_changed || force)
  // {
  //   tft.fillScreen(TFT_BLACK);
  //   char buff[4];
  //   sprintf(buff, "%3d", board.num_times_controller_offline);
  //   chunkyDrawFloat(MC_DATUM, buff, NULL, 5, 10);
  //   lcd_message(TC_DATUM, "Board missed", TFT_WIDTH/2, (TFT_HEIGHT/2) + FONT_1_HEIGHT, 1);
  //   // board restart count
  //   char buff1[20];
  //   sprintf(buff1, "Board reset: %d", board_first_packet_count);
  //   lcd_bottom_line(buff1);
  // }
  // old_board_first_packet_count = board_first_packet_count;
}

void screen_not_moving(uint8_t trigger_state)
{
  char buff2[10];
  sprintf(buff2, "trig: %d", trigger_state);
  
  u8g2.clearBuffer();
  // line 1
  lcd_message(/*line#*/ 1, "Stopped");
  //line 2
  char buff3[12];
#ifdef USE_DEADMAN_SWITCH  
  switch (trigger_state)
  {
    case 0: lcd_message(/*line*/ 2, "trig: go!"); break;
    case 1: lcd_message(/*line*/ 2, "trig: wait"); break;
    case 2: lcd_message(/*line*/ 2, "trig: hold"); break;
  }
#endif
  sprintf(buff3, "bd rsts: %d", board_first_packet_count);
  lcd_message(/*line#*/ 3, &buff3[0]);
  u8g2.sendBuffer();
}