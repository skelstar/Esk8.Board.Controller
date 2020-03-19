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
  lcd_message(buff, LINE_3, Aligned::ALIGNED_LEFT);

  drawSmallBattery(remote_battery_percent, LCD_WIDTH - MARGIN, 0 + MARGIN, TR_DATUM);

  if (!connected)
  {
    tft.drawRect(0, 0, LCD_WIDTH, LCD_HEIGHT, TFT_RED);
  }
}
//-----------------------------------------------------

void screen_moving()
{
  uint32_t bgColour = TFT_DARKGREEN;
  tft.fillScreen(TFT_DARKGREEN);

  char buff[10];

  // line 1 amps

  // line 2+ throttle
  ChunkyDigit chunky_digit(&tft, 10, 8, bgColour);
  sprintf(buff, "%d", controller_packet.throttle);
  chunky_digit.draw_float(BR_DATUM, buff, "Thr");
}
//-----------------------------------------------------
void screenShowOptionWithValue(char *title, OptionValue *opt)
{
  tft.fillScreen(opt->bgcolour);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(title, LCD_WIDTH / 2, 20);

  ChunkyDigit chunky_digit(&tft, 10, 10, opt->bgcolour);

  char buff[6];
  sprintf(buff, "%d", opt->get());
  chunky_digit.draw_float(MC_DATUM, buff, NULL);
}
//-----------------------------------------------------
void screenShowOptionValueSelected()
{
  tft.setTextDatum(MC_DATUM);
  tft.drawString("selected!", LCD_WIDTH / 2, LCD_HEIGHT - 20);
}
//-----------------------------------------------------