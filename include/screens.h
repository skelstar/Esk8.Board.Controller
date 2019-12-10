

void draw_missing_packets_screen()
{
  bool something_changed = vescdata.ampHours != old_vescdata.ampHours ||
    board_first_packet_count != old_board_first_packet_count;

  if (something_changed)
  {
    tft.fillScreen(TFT_BLACK);
    char buff[4];
    sprintf(buff, "%3d", board.num_times_controller_offline);
    chunkyDrawFloat(MC_DATUM, buff, NULL, 5, 10);
    lcd_message(TC_DATUM, "Board missed", TFT_WIDTH/2, (TFT_HEIGHT/2) + FONT_1_HEIGHT, 1);
    // board restart count
    char buff1[20];
    sprintf(buff1, "Board reset: %d", board_first_packet_count);
    lcd_bottom_line(buff1);
  }
  old_board_first_packet_count = board_first_packet_count;
}