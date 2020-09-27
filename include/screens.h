#include <ChunkyDigit.h>
#include <math.h>

#include <WidgetClass.h>

#ifndef TFT_H
#include <tft.h>
#endif

#define LINE_1 1
#define LINE_2 2
#define LINE_3 3
#define LINE_4 4

#define CHUNKY_PIXEL_SML 4
#define CHUNKY_PIXEL_MED 10
#define CHUNKY_SPACING_SML 4
#define CHUNKY_SPACING_MED 8

#define STRIPE_HEIGHT 10

// prototypes
void setupScreen(uint32_t bgColour, uint32_t fgColour, uint32_t stripeColour);

//-----------------------------------------------------

void screen_searching()
{
  tft.setTextDatum(TL_DATUM);
  tft.fillScreen(TFT_DEFAULT_BG);

  tft.setFreeFont(FONT_LG);
  tft.setTextSize(1);

  uint16_t x = getX(ALIGNED_CENTRE),
           y = getY(LINE_1);
  tft.drawString("Searching...", x, y);

  y = 40;

  tft.setTextColor(TFT_DARKGREY);

  // draw remote battery

  BatteryLib remote_batt(BATTERY_MEASURE_PIN);

  uint8_t w = 40,
          h = 20,
          x1 = LCD_WIDTH - w - 5, /*margin*/
      y1 = 5;                     /*margin*/
  remote_batt.setup(NULL);
  remote_batt.update();

  drawBattery(remote_batt, x1, y1, w, h, TFT_DARKGREY);

  if (FEATURE_CRUISE_CONTROL)
  {
    tft.setTextDatum(TC_DATUM);
    y += 30;
    tft.drawString("- cruise ctrl", 10, y);
  }

  if (FEATURE_PUSH_TO_START)
  {
    tft.setTextDatum(TC_DATUM);
    y += 30;
    tft.drawString("- push to start", 10, y);
  }
}
//-----------------------------------------------------
void screenWhenDisconnected()
{
  setupScreen(/*bg*/ TFT_DEFAULT_BG, /*fg*/ TFT_WHITE, TFT_RED);

  // line 1
  char buff1[20];
  sprintf(buff1, "board resets: %d", stats.boardResets);
  lcd_message(buff1, LINE_1, Aligned::ALIGNED_LEFT, FontSize::LG);

  // line 2
  char buff2[20];
  sprintf(buff2, "failed tx: %d", stats.total_failed_sending);
  lcd_message(buff2, LINE_2, Aligned::ALIGNED_LEFT, FontSize::LG);

  // line 3
  char buff3[20];
  sprintf(buff3, "trip Ah: %.1f", board.packet.ampHours);
  lcd_message(buff3, LINE_3, Aligned::ALIGNED_LEFT, FontSize::LG);

  // line 4
  char buff4[20];
  int timeMins = (sinceBoardConnected / 1000) / 60;
  sprintf(buff4, "time (mins): %d", timeMins);
  lcd_message(buff4, LINE_4, Aligned::ALIGNED_LEFT, FontSize::LG);
}
//-----------------------------------------------------

void screen_with_stats(bool connected = true)
{
  uint32_t bgColour = connected ? TFT_DEFAULT_BG : TFT_RED;
  tft.fillScreen(bgColour);
  // battery
  drawSmallBattery(remote_battery_percent, LCD_WIDTH - MARGIN, 0 + MARGIN, TR_DATUM);
  // line 1
  char buff1[20];
  sprintf(buff1, "bd rsts: %d", stats.boardResets);
  lcd_message(buff1, LINE_1, Aligned::ALIGNED_LEFT, FontSize::LG, getStatus(stats.boardResets, 0, 1, 1));
  // line 2
  char buff2[20];
  sprintf(buff2, "failed tx: %d", stats.total_failed_sending);
  lcd_message(buff2, LINE_2, Aligned::ALIGNED_LEFT, FontSize::LG, getStatus(stats.total_failed_sending, 0, 1, 2));
  // line 3
}
//-----------------------------------------------------

ChunkyDigit *chunkyDigit;
//-----------------------------------------------------
void screenPropValue(char *propName, const char *value)
{
  tft.fillScreen(TFT_DEFAULT_BG);
  lcd_messageBase(propName, /*y*/ 20, Aligned::ALIGNED_CENTRE, FontSize::LG, TFT_DARKGREY);

  chunkyDigit = new ChunkyDigit(&tft, CHUNKY_PIXEL_MED, CHUNKY_SPACING_MED, TFT_DEFAULT_BG);

  int w = chunkyDigit->getWidth((const char *)value);
  chunkyDigit->drawText(value, LCD_WIDTH / 2 - w / 2, LCD_HEIGHT - chunkyDigit->getHeight() - 30);
}
//-----------------------------------------------------

void screenOneMetricWithStripe(float value, char *title, uint32_t stripeColour, bool init)
{
  uint8_t x_right = 200,
          y = 30;

  // setup
  if (init || chunkyDigit == NULL)
  {
    setupScreen(/*bg*/ TFT_DEFAULT_BG, /*fg*/ TFT_WHITE, stripeColour);

    tft.setFreeFont(FONT_MED);
    tft.setTextColor(TFT_DARKGREY);
    tft.setTextDatum(TL_DATUM);
    tft.drawString(title, x_right - tft.textWidth(title), y);

    chunkyDigit = new ChunkyDigit(&tft, CHUNKY_PIXEL_MED, CHUNKY_SPACING_MED, TFT_DEFAULT_BG);
  }

  char buff[20];
  sprintf(buff, value < 1000.0 ? "%.1f" : "%.0f", value);

  // blank numbers
  tft.fillRect(0, y + 25, LCD_WIDTH, LCD_HEIGHT - (y + 25), TFT_DEFAULT_BG);

  int w = chunkyDigit->getWidth(buff);
  chunkyDigit->draw_float(x_right - w, /*y*/ y + 25, buff);
}

enum QuarterPosition
{
  TOP_LEFT_QRTR,
  TOP_RIGHT_QRTR,
  BOTTOM_LEFT_QRTR,
  BOTTOM_RIGHT_QRTR
};

int getQuarterX(QuarterPosition pos)
{
  switch (pos)
  {
  case TOP_LEFT_QRTR:
  case BOTTOM_LEFT_QRTR:
    return 0;
  case TOP_RIGHT_QRTR:
  case BOTTOM_RIGHT_QRTR:
    return LCD_WIDTH / 2;
  }
}

int getQuarterY(QuarterPosition pos)
{
  switch (pos)
  {
  case TOP_LEFT_QRTR:
  case TOP_RIGHT_QRTR:
    return STRIPE_HEIGHT;
  case BOTTOM_LEFT_QRTR:
  case BOTTOM_RIGHT_QRTR:
    return (LCD_HEIGHT - STRIPE_HEIGHT) / 2 + STRIPE_HEIGHT;
  }
}

void quarterScreen(QuarterPosition position, char *buff, char *title)
{
  int x = getQuarterX(position);
  int y = getQuarterY(position);
  int qrtrHeight = (LCD_HEIGHT - STRIPE_HEIGHT) / 2;
  // tft.drawRect(x, y, LCD_WIDTH / 2, qrtrHeight, TFT_DARKGREY);

  const uint8_t RIGHT_MARGIN = 5;
  const uint8_t TOP_MARGIN = 2;
  const uint8_t BOTTOM_MARGIN = 5;
  tft.setTextDatum(TR_DATUM);
  tft.drawString(title, x + (LCD_WIDTH / 2) - RIGHT_MARGIN, y + TOP_MARGIN);

  chunkyDigit = new ChunkyDigit(&tft, /*pixel*/ 6, /*spacing*/ 5, TFT_DEFAULT_BG);

  int w = chunkyDigit->getWidth(buff);
  chunkyDigit->draw_float(x + (LCD_WIDTH / 2) - w - RIGHT_MARGIN, y + qrtrHeight - chunkyDigit->getHeight() - BOTTOM_MARGIN, buff);
}

void quarterScreenFloat(QuarterPosition position, float value, char *title)
{
  char buff[20];
  sprintf(buff, value < 1000.0 ? "%.1f" : "%.0f", value);

  quarterScreen(position, buff, title);
}
//-----------------------------------------------------

void screenWhenStopped(bool init = false)
{
  if (init || chunkyDigit == NULL)
  {
    setupScreen(/*bg*/ TFT_DEFAULT_BG, /*fg*/ TFT_WHITE, TFT_DARKGREY);
    tft.setFreeFont(FONT_MED);
  }
  quarterScreenFloat(TOP_LEFT_QRTR, board.packet.odometer, "trip (km)");
  quarterScreenFloat(TOP_RIGHT_QRTR, stats.timeMovingMS / (60 * 1000.0), "time (m)");
  quarterScreenFloat(BOTTOM_LEFT_QRTR, board.packet.ampHours, "mAH");
  quarterScreenFloat(BOTTOM_RIGHT_QRTR, stats.getAverageAmpHours(board.packet.ampHours), "mAH/s");
  // screenOneMetricWithStripe(board.packet.ampHours, "TRIP Ah", TFT_DARKGREY, init);
}
//-----------------------------------------------------

void screenWhenMoving(bool init = false)
{
  screenOneMetricWithStripe(board.packet.motorCurrent, "MOTOR AMPS", TFT_DARKGREEN, init);
}
//-----------------------------------------------------
void screenTogglePushToStart()
{
}

void setupScreen(uint32_t bgColour, uint32_t fgColour, uint32_t stripeColour)
{
  tft.fillScreen(bgColour);
  tft.fillRect(0, 0, LCD_WIDTH, STRIPE_HEIGHT, stripeColour);
  _spr.setTextColor(fgColour);
  _spr.setFreeFont(FONT_MED);
  _spr.createSprite(LCD_WIDTH, LCD_HEIGHT - 50);
}
