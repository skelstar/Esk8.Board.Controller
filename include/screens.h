#include <ChunkyDigit.h>
#include <SlantyDigit.h>
#include <math.h>
#include <string.h>

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

namespace Display
{

  // prototypes
  void drawStatusStripe(uint32_t bgColour, uint32_t fgColour, uint32_t stripeColour);

  ChunkyDigit *chunkyDigit;
  SlantyDigit *slantyDigit;

  //-----------------------------------------------------
  void screen_searching()
  {
    //if (mutex_SPI.take(__func__))
    // {
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
    // mutex_SPI.give(__func__);
    // }
  }
  //-----------------------------------------------------
  void screenWhenDisconnected()
  {
    //if (mutex_SPI.take(__func__))
    // {
    tft.setFreeFont(FONT_LG);
    tft.setTextSize(1);

    drawStatusStripe(/*bg*/ TFT_DEFAULT_BG, /*fg*/ TFT_WHITE, TFT_RED);

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

    // mutex_SPI.give(__func__);
    // }
  }
  //-----------------------------------------------------

  void screenSoftwareStats()
  {
    //if (mutex_SPI.take(__func__))
    // {
    tft.fillScreen(TFT_DEFAULT_BG);
    tft.setFreeFont(FONT_LG);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    const int lineHeight = tft.fontHeight() + 3;
    int line1 = MARGIN, line2 = line1 + lineHeight, xmargin = 15;

    char branch[30];
    sprintf(branch, "%s", GIT_BRANCH_NAME);
    char build[30];
    sprintf(build, "%s", DEBUG_BUILD ? "DEBUG" : "RELEASE");

    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("br: ", MARGIN, line1);
    tft.drawString("bld: ", MARGIN, line2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(branch, MARGIN + tft.textWidth("br: ") + xmargin, line1);
    tft.drawString(build, MARGIN + tft.textWidth("bld: ") + xmargin, line2);

    // mutex_SPI.give(__func__);
    // }
  }

#define BATTERY_WIDTH (LCD_WIDTH / 8) * 6
#define BATTERY_HEIGHT (LCD_HEIGHT / 8) * 6
#define BORDER_SIZE 12
#define KNOB_HEIGHT (BATTERY_HEIGHT / 8) * 4

  void screenBoardBattery(float batteryVolts)
  {
    //if (mutex_SPI.take(__func__))
    // {
    uint8_t percent = getBatteryPercentage(batteryVolts);
    tft.fillScreen(TFT_DEFAULT_BG);
    int outsideX = (LCD_WIDTH - (BATTERY_WIDTH + BORDER_SIZE)) / 2; // includes batt knob
    int outsideY = (LCD_HEIGHT - BATTERY_HEIGHT) / 2;
    // body
    tft.fillRect(outsideX, outsideY, BATTERY_WIDTH, BATTERY_HEIGHT, TFT_WHITE);
    // knob
    tft.fillRect(
        outsideX + BATTERY_WIDTH,
        outsideY + (BATTERY_HEIGHT - KNOB_HEIGHT) / 2,
        BORDER_SIZE,
        KNOB_HEIGHT,
        TFT_WHITE);
    // inside
    tft.fillRect(
        outsideX + BORDER_SIZE,
        outsideY + BORDER_SIZE,
        BATTERY_WIDTH - BORDER_SIZE * 2,
        BATTERY_HEIGHT - BORDER_SIZE * 2,
        TFT_DEFAULT_BG);
    // capacity
    uint32_t colour = percent > 50
                          ? TFT_WHITE
                      : percent > 20
                          ? TFT_ORANGE
                          : TFT_RED;
    tft.fillRect(
        outsideX + BORDER_SIZE * 2,
        outsideY + BORDER_SIZE * 2,
        (BATTERY_WIDTH - BORDER_SIZE * 4) * percent / 100,
        BATTERY_HEIGHT - BORDER_SIZE * 4,
        colour);

    // mutex_SPI.give(__func__);
    // }
  }
  //-----------------------------------------------------
  template <typename T>
  void screenPropValue(char *propName, const char *value)
  {
    //if (mutex_SPI.take(__func__))
    // {
    tft.fillScreen(TFT_DEFAULT_BG);
    lcd_messageBase(propName, /*y*/ 20, Aligned::ALIGNED_CENTRE, FontSize::LG, TFT_DARKGREY);

    chunkyDigit = new ChunkyDigit(&tft, CHUNKY_PIXEL_MED, CHUNKY_SPACING_MED, TFT_DEFAULT_BG);

    int w = chunkyDigit->getWidth((const char *)value);

    int x = LCD_WIDTH / 2 - w / 2,
        y = LCD_HEIGHT - chunkyDigit->getHeight() - 30;

    if (std::is_same<T, int>::value ||
        std::is_same<T, uint8_t>::value ||
        std::is_same<T, uint16_t>::value ||
        std::is_same<T, float>::value)
      chunkyDigit->draw_float(x, y, (char *)value);
    else if (std::is_same<T, bool>::value)
      chunkyDigit->drawText(value, x, y);
    else
      chunkyDigit->drawText("-", x, y);

    tft.setFreeFont(FONT_SM);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_DARKGREY);
    tft.drawString("long hold | tap ->", LCD_WIDTH / 2, LCD_HEIGHT - 5);

    // mutex_SPI.give(__func__);
    // }
  }
  //-----------------------------------------------------

  void screenOneMetricWithStripe(float value, char *title, uint32_t stripeColour, bool init, uint32_t bgColour)
  {
    //if (mutex_SPI.take(__func__))
    // {
    uint8_t x_right = 200,
            y = 30;

    // setup
    if (init || chunkyDigit == NULL)
    {
      drawStatusStripe(/*bg*/ bgColour, /*fg*/ TFT_WHITE, stripeColour);

      tft.setFreeFont(FONT_MED);
      tft.setTextColor(TFT_DARKGREY);
      tft.setTextDatum(TL_DATUM);
      tft.drawString(title, x_right - tft.textWidth(title), y);

      chunkyDigit = new ChunkyDigit(&tft, CHUNKY_PIXEL_MED, CHUNKY_SPACING_MED, bgColour);
    }

    char buff[20];
    sprintf(buff, value < 1000.0 ? "%.1f" : "%.0f", value);

    // blank numbers
    tft.fillRect(0, y + 25, LCD_WIDTH, LCD_HEIGHT - (y + 25), bgColour);

    int w = chunkyDigit->getWidth(buff);
    chunkyDigit->draw_float(x_right - w, /*y*/ y + 25, buff);

    // mutex_SPI.give(__func__);
    // }
  }

  //-----------------------------------------------------

  void simpleStoppedScreen(const char *text, uint32_t colour)
  {
    //if (mutex_SPI.take(__func__))
    // {
    uint8_t y = 0;
    tft.fillScreen(TFT_BLACK);

    y = (LCD_HEIGHT / 2) - tft.fontHeight() - 15;
    tft.setFreeFont(FONT_LG);
    tft.setTextColor(TFT_CYAN);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(text, LCD_WIDTH / 2, y);
    y += tft.fontHeight();

    // remote battery
    const int batteryWidth = 50;
    if (Remote::mutex.take(__func__, TICKS_50))
    {
      y += 15;
      uint8_t percent = Remote::battery.chargePercent;
      Remote::mutex.give(__func__);
      drawSmallBattery(percent, LCD_WIDTH / 2 - 5, y, batteryWidth, TR_DATUM, Remote::battery.isCharging);

      char buff[10];
      sprintf(buff, "%0.1fv", Remote::battery.getVolts());
      tft.setTextDatum(TL_DATUM);
      tft.setFreeFont(FONT_LG);
      tft.setTextColor(TFT_DARKGREY);
      tft.drawString(buff, LCD_WIDTH / 2 + 5, y - 4);
    }

    y += 30;
    tft.setTextColor(TFT_DARKGREY);

    tft.setFreeFont(FONT_MED);
    tft.setTextDatum(TC_DATUM);
    std::string s(GIT_BRANCH_NAME);
    if (s.length() > 20)
      s = s.substr(0, 20) + "..."; // truncate and add "..."
    tft.drawString(s.c_str(), LCD_WIDTH / 2, y);

    // mutex_SPI.give(__func__);
    // }
  }
  //-----------------------------------------------------

  void simpleMovingScreen()
  {
    //if (mutex_SPI.take(__func__))
    // {
    uint8_t y = 0;
    tft.fillScreen(TFT_BLACK);

    y = (LCD_HEIGHT / 2);
    tft.setFreeFont(FONT_LG);
    tft.setTextColor(TFT_CYAN);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("MOVING", LCD_WIDTH / 2, y);
    y += tft.fontHeight();

    // mutex_SPI.give(__func__);
    // }
  }
  //-----------------------------------------------------

  void screenWhenMoving(bool init = false)
  {
    //if (mutex_SPI.take(__func__))
    // {
    screenOneMetricWithStripe(
        board.packet.motorCurrent,
        "MOTOR AMPS",
        /*stripe*/ TFT_DARKGREEN,
        init,
        TFT_DEFAULT_BG);

    // mutex_SPI.give(__func__);
    // }
  }
  //-----------------------------------------------------

  void screenNeedToAckResets(Stats::ResetsType type)
  {
    //if (mutex_SPI.take(__func__))
    // {
    char buff[10];
    uint32_t bgColour = 0;
    if (type == Stats::CONTROLLER_RESETS)
    {
      bgColour = TFT_RED;
      sprintf(buff, "%d", stats.controllerResets);
    }
    else if (type == Stats::BOARD_RESETS)
    {
      bgColour = TFT_NAVY;
      sprintf(buff, "%d", stats.boardResets);
    }
    else
    {
      return;
    }

    tft.fillScreen(bgColour);
    tft.setFreeFont(FONT_LG);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("ACK RESETS!\n", /*x*/ LCD_WIDTH / 2, /*y*/ 20);

    chunkyDigit = new ChunkyDigit(&tft, CHUNKY_PIXEL_MED, CHUNKY_SPACING_MED, bgColour);

    int w = chunkyDigit->getWidth(buff);
    int x = LCD_WIDTH / 2 - w / 2;
    int y = LCD_HEIGHT - chunkyDigit->getHeight() - 20;
    chunkyDigit->draw_float(x, y, buff);

    // mutex_SPI.give(__func__);
    // }
  }
  //-----------------------------------------------------

  void screenBoardNotCompatible(float boardVersion)
  {
    //if (mutex_SPI.take(__func__))
    // {
    tft.fillScreen(TFT_NAVY);
    tft.setFreeFont(FONT_LG);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(TC_DATUM);
    uint8_t line1 = 20, lineheight = 32;
    tft.drawString("Board not", /*x*/ LCD_WIDTH / 2, /*y*/ line1);
    tft.drawString("compatible!", /*x*/ LCD_WIDTH / 2, /*y*/ line1 += lineheight);

    char buff[20];
    sprintf(buff, "is v%.1f not v%.1f", boardVersion, VERSION_BOARD_COMPAT);
    tft.drawString(buff, /*x*/ LCD_WIDTH / 2, /*y*/ line1 += lineheight);

    // mutex_SPI.give(__func__);
    // }
  }

  //-----------------------------------------------------

  void drawStatusStripe(uint32_t bgColour, uint32_t fgColour, uint32_t stripeColour)
  {
    //if (mutex_SPI.take(__func__))
    // {
    tft.fillScreen(bgColour);
    tft.fillRect(0, 0, LCD_WIDTH, STRIPE_HEIGHT, stripeColour);

    // mutex_SPI.give(__func__);
    // }
  }
} // namespace