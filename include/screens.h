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
  int y = 40;
  tft.drawString("interval: ", 10, y);
  tft.drawNumber(SEND_TO_BOARD_INTERVAL, tft.textWidth("interval: ") + 10, y);
#ifdef FEATURE_CRUISE_CONTROL
  tft.setTextDatum(TC_DATUM);
  y += 30;
  tft.drawString("cruise ctrl", 10, y);
#endif
#ifdef PUSH_TO_START
  tft.setTextDatum(TC_DATUM);
  y += 30;
  tft.drawString("push to start", 10, y);
#endif
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
  if (board.packet.missedPackets > 0 || board.packet.unsuccessfulSends > 0)
  {
    char buff3_1[12];
    sprintf(buff3_1, "bd ms:%d", board.packet.missedPackets);
    lcd_message(buff3_1, LINE_3, Aligned::ALIGNED_LEFT, getStatus(board.packet.missedPackets, 0, 1, 2));
    char buff3_2[12];
    sprintf(buff3_2, "us:%d", board.packet.unsuccessfulSends);
    lcd_message(buff3_2, LINE_3, Aligned::ALIGNED_RIGHT, getStatus(board.packet.unsuccessfulSends, 0, 1, 2));
  }
}
//-----------------------------------------------------

WidgetClass<uint8_t> *widgetRsts;
WidgetClass<uint16_t> *widgetFail;
WidgetClass<uint16_t> *widgetThrottle;
WidgetClass<uint16_t> *widgetMissed;
WidgetClass<uint16_t> *widgetUnsuccessful;
WidgetClass<float> *widgetVolts;

void initWidgets()
{
  widgetRsts = new WidgetClass<uint8_t>(WidgetPos::TOP_LEFT, WidgetSize::Normal);
  widgetRsts->setStatusLevels(1, 1);
  widgetRsts->setOnlyShowNonZero(true);

  widgetFail = new WidgetClass<uint16_t>(WidgetPos::TOP_CENTRE, WidgetSize::Normal);
  widgetFail->setStatusLevels(2, 5);
  widgetFail->setOnlyShowNonZero(true);

  widgetThrottle = new WidgetClass<uint16_t>(WidgetPos::TOP_RIGHT, WidgetSize::Normal, TFT_GREEN, TFT_BLACK);

  widgetMissed = new WidgetClass<uint16_t>(WidgetPos::BOTTOM_LEFT, WidgetSize::Normal);
  widgetMissed->setStatusLevels(1, 3);
  widgetMissed->setOnlyShowNonZero(true);

  widgetUnsuccessful = new WidgetClass<uint16_t>(WidgetPos::BOTTOM_CENTRE, WidgetSize::Normal);
  widgetUnsuccessful->setStatusLevels(1, 3);
  widgetUnsuccessful->setOnlyShowNonZero(true);

  widgetVolts = new WidgetClass<float>(WidgetPos::BOTTOM_RIGHT, WidgetSize::Normal);
  widgetVolts->setStatusLevels(/*warn*/ 37.4, /*crit*/ 35.0, /*swapped*/ true);
}

void screenWithWidgets(bool connected = true)
{
  widgetRsts->draw(stats.boardResets, "RSTS");
  widgetFail->draw(stats.total_failed_sending, "FAIL");
  widgetThrottle->draw(controller_packet.throttle, "THR");
  widgetMissed->draw(board.packet.missedPackets, "MISSED");
  widgetUnsuccessful->draw(board.packet.unsuccessfulSends, "SENDS");
  widgetVolts->draw(board.packet.batteryVoltage, "BATT");
}
//-----------------------------------------------------

void screen_moving()
{
  uint32_t bgColour = TFT_DARKGREEN;
  tft.fillScreen(TFT_DARKGREEN);

  char buff[10];

  // line 1 amps
  ChunkyDigit chunkAmps(&tft, 10, 8, bgColour);
  sprintf(buff, "%.1f", board.packet.motorCurrent);
  chunkAmps.draw_float(TC_DATUM, ChunkyDigit::LINE1_OF_2, buff, "Ah");

  // line 2 throttle
  ChunkyDigit chunky_digit(&tft, 10, 8, bgColour);
  sprintf(buff, "%d", controller_packet.throttle);
  chunkAmps.draw_float(BC_DATUM, ChunkyDigit::LINE2_OF_2, buff, "Thr");
}
//-----------------------------------------------------