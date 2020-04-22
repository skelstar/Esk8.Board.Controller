#include <ChunkyDigit.h>
#include <math.h>

#include <WidgetClass.h>

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
  uint32_t bgColour = connected ? TFT_BLUE : TFT_RED;
  tft.fillScreen(bgColour);
  // battery
  drawSmallBattery(remote_battery_percent, LCD_WIDTH - MARGIN, 0 + MARGIN, TR_DATUM);
  // line 1
  char buff1[20];
  sprintf(buff1, "bd rsts: %d", stats.boardResets);
  lcd_message(buff1, LINE_1, Aligned::ALIGNED_LEFT, getStatus(stats.boardResets, 0, 1, 1));
  // line 2
  char buff2[20];
  sprintf(buff2, "failed tx: %lu", stats.total_failed_sending);
  lcd_message(buff2, LINE_2, Aligned::ALIGNED_LEFT, getStatus(stats.total_failed_sending, 0, 1, 2));
  // line 3
  if (board_packet.missedPackets > 0 || board_packet.unsuccessfulSends > 0)
  {
    char buff3_1[12];
    sprintf(buff3_1, "bd ms:%d", board_packet.missedPackets);
    lcd_message(buff3_1, LINE_3, Aligned::ALIGNED_LEFT, getStatus(board_packet.missedPackets, 0, 1, 2));
    char buff3_2[12];
    sprintf(buff3_2, "us:%d", board_packet.unsuccessfulSends);
    lcd_message(buff3_2, LINE_3, Aligned::ALIGNED_RIGHT, getStatus(board_packet.unsuccessfulSends, 0, 1, 2));
  }
}
//-----------------------------------------------------

void screenWithWidgets(bool connected = true)
{
  tft.fillScreen(connected ? TFT_BLUE : TFT_RED);

  WidgetClass<uint8_t> *widgetRsts = new WidgetClass<uint8_t>();
  widgetRsts->setPosition(WidgetPos::TOP_LEFT, WidgetSize::Normal);
  widgetRsts->setStatusLevels(1, 1);
  widgetRsts->draw(&tft, stats.boardResets, "RSTS");

  WidgetClass<uint16_t> *widgetFail = new WidgetClass<uint16_t>();
  widgetFail->setPosition(WidgetPos::TOP_CENTRE, WidgetSize::Normal);
  widgetFail->setStatusLevels(2, 5);
  widgetFail->draw(&tft, stats.total_failed_sending, "FAIL");

  WidgetClass<uint16_t> *widgetThrottle = new WidgetClass<uint16_t>(TFT_GREEN);
  widgetThrottle->setForegroundColour(TFT_BLACK);
  widgetThrottle->setPosition(WidgetPos::TOP_RIGHT, WidgetSize::Normal);
  widgetThrottle->draw(&tft, controller_packet.throttle, "THR");

  if (board_packet.missedPackets > 0)
  {
    WidgetClass<uint16_t> *widgetMissed = new WidgetClass<uint16_t>();
    widgetMissed->setPosition(WidgetPos::BOTTOM_LEFT, WidgetSize::Normal);
    widgetMissed->setStatusLevels(1, 3);
    widgetMissed->draw(&tft, board_packet.missedPackets, "MISSED");
  }
  if (board_packet.unsuccessfulSends > 0)
  {
    WidgetClass<uint16_t> *widgetUnsuccessful = new WidgetClass<uint16_t>();
    widgetUnsuccessful->setPosition(WidgetPos::BOTTOM_CENTRE, WidgetSize::Normal);
    widgetUnsuccessful->setStatusLevels(1, 3);
    widgetUnsuccessful->draw(&tft, board_packet.unsuccessfulSends, "SENDS");
  }
  // battery
  WidgetClass<float> *widgetVolts = new WidgetClass<float>();
  widgetVolts->setPosition(WidgetPos::BOTTOM_RIGHT, WidgetSize::Normal);
  widgetVolts->setStatusLevels(/*warn*/ 37.4, /*crit*/ 35.0, /*swapped*/ true);
  widgetVolts->draw(&tft, board_packet.batteryVoltage, "BATT");
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