#include <ChunkyDigit.h>
#include <math.h>

#ifndef TFT_H
#include <tft.h>
#endif

#define LINE_1 1
#define LINE_2 2
#define LINE_3 3

// prototypes

//-----------------------------------------------------

void screen_searching()
{
  tft.setTextDatum(TL_DATUM);
  tft.fillScreen(TFT_BLUE);

  lcd_message("Searching...", LINE_1, Aligned::ALIGNED_CENTRE);
  // send interval
  tft.drawString("interval: ", 10, 40);
  tft.drawNumber(SEND_TO_BOARD_INTERVAL, tft.textWidth("interval: ") + 10, 40);

  uint8_t y = 70;
  char buff[30];
  sprintf(buff, "enc: -%d->%d", config.brakeCounts, config.accelCounts);
  tft.drawString(buff, 10, y);
}
//-----------------------------------------------------

MessageStatus getStatus(uint8_t val, uint8_t okay, uint8_t warn, uint8_t crit)
{
  if (val >= crit)
  {
    return CRITICAL;
  }
  else if (val >= warn)
  {
    return WARNING;
  }
  return OKAY;
}

void screen_with_stats(bool connected = true)
{
  tft.fillScreen(TFT_BLUE);
  // line 1
  char buff1[20];
  sprintf(buff1, "rsts: %d", stats.soft_resets);
  lcd_message(buff1, LINE_1, Aligned::ALIGNED_LEFT, getStatus(stats.soft_resets, 0, 1, 1));
  // line 2
  char buff2[20];
  sprintf(buff2, "total f: %lu", stats.total_failed);
  lcd_message(buff2, LINE_2, Aligned::ALIGNED_LEFT, getStatus(stats.total_failed, 0, 1, 2));
  // line 3
  char buff[30];
  sprintf(buff, "enc: -%d->%d", config.brakeCounts, config.accelCounts);
  uint8_t y = 135 - 30;
  lcd_message(buff, LINE_3, Aligned::ALIGNED_LEFT);

  // char buff3[20];
  // sprintf(buff3, "w/rt: %lu", stats.num_packets_with_retries);
  // lcd_message(buff3, LINE_3, Aligned::ALIGNED_LEFT, getStatus(stats.num_packets_with_retries, 0, 10, 50));
  // battery
  draw_small_battery(remote_battery_percent, LCD_WIDTH - MARGIN, 0 + MARGIN, TR_DATUM);

  if (!connected)
  {
    tft.drawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, TFT_RED);
  }
}
//-----------------------------------------------------

void screen_moving()
{
  char buff[10];

  // ChunkyDigit chunky_digit(&tft, 6, 3);

  //u8g2.clearBuffer();
  // sprintf(buff, "%.1f%%", retry_log.get());
  // chunky_digit.draw_float(TR_DATUM, buff, NULL);
  // draw_trigger_state(throttle.get_current_state(), BR_DATUM);
  //u8g2.sendBuffer();
}
//-----------------------------------------------------