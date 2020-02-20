#include <ChunkyDigit.h>
#include <math.h>

#ifndef TFT_H
#include <tft.h>
#endif

#define LINE_1 1
#define LINE_2 2
#define LINE_3 3

// prototypes
void screen_with_stats();

//-----------------------------------------------------

void screen_searching()
{
  tft.fillScreen(TFT_BLUE);
  lcd_message("Searching...", LINE_2, Aligned::ALIGNED_CENTRE);
}
//-----------------------------------------------------

void screen_disconnected()
{
  screen_with_stats();
}
//-----------------------------------------------------

void screen_with_stats()
{
  tft.fillScreen(TFT_BLUE);
  // line 1
  char buff1[20];
  float retry_rate = retry_log.get();
  sprintf(buff1, "rate: %.1f%%", isnan(retry_rate) ? 0.0 : retry_rate);
  lcd_message(buff1, LINE_1, Aligned::ALIGNED_LEFT);
  // line 2
  char buff2[20];
  sprintf(buff2, "total: %lu", stats.total_failed);
  lcd_message(buff2, LINE_2, Aligned::ALIGNED_LEFT);
  // line 3
  char buff3[20];
  sprintf(buff3, "w/rt: %lu", stats.num_packets_with_retries);
  lcd_message(buff3, LINE_3, Aligned::ALIGNED_LEFT);
  // battery
  draw_small_battery(remote_battery_percent, 128, 0, TR_DATUM);
  // deadman
  draw_trigger_state(trigger.get_current_state(), BR_DATUM);
}
//-----------------------------------------------------

void screen_moving()
{
  char buff[10];

  // ChunkyDigit chunky_digit(&tft, 6, 3);

  //u8g2.clearBuffer();
  // sprintf(buff, "%.1f%%", retry_log.get());
  // chunky_digit.draw_float(TR_DATUM, buff, NULL);
  // draw_trigger_state(trigger.get_current_state(), BR_DATUM);
  //u8g2.sendBuffer();
}
//-----------------------------------------------------