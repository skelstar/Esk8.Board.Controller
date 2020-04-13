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
}
//-----------------------------------------------------

void screen_with_stats(bool connected = true)
{
  tft.fillScreen(TFT_BLUE);
  // line 1
  char buff1[20];
  sprintf(buff1, "bd rsts: %d", stats.boardResets);
  lcd_message(buff1, LINE_1, Aligned::ALIGNED_LEFT, getStatus(stats.boardResets, 0, 1, 1));
  // line 2
  char buff2[20];
  sprintf(buff2, "total f: %lu", stats.total_failed);
  lcd_message(buff2, LINE_2, Aligned::ALIGNED_LEFT, getStatus(stats.total_failed, 0, 1, 2));
  // line 3

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
  ChunkyDigit chunkAmps(&tft, 10, 8, bgColour);
  sprintf(buff, "%.1f", board_packet.motorCurrent);
  chunkAmps.draw_float(TC_DATUM, ChunkyDigit::LINE1_OF_2, buff, "Ah");

  // line 2 throttle
  ChunkyDigit chunky_digit(&tft, 10, 8, bgColour);
  sprintf(buff, "%d", controller_packet.throttle);
  chunkAmps.draw_float(BC_DATUM, ChunkyDigit::LINE2_OF_2, buff, "Thr");
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
  chunky_digit.draw_float(MC_DATUM, buff);
}
//-----------------------------------------------------
void screenShowOptionWithValue(char *title, char *opt, uint32_t bgcolour)
{
  tft.fillScreen(bgcolour);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(title, LCD_WIDTH / 2, 20);

  tft.setTextDatum(MC_DATUM);
  tft.drawString(opt, LCD_WIDTH / 2, 80);
}
//-----------------------------------------------------
void screenShowOptionValueSelected()
{
  tft.setTextDatum(MC_DATUM);
  tft.drawString("selected!", LCD_WIDTH / 2, LCD_HEIGHT - 20);
}
//-----------------------------------------------------