#pragma once

#ifndef TFT_eSPI
#include <TFT_eSPI.h>
#endif

#include <BatteryLib.h>

// #include <fonts/Custom/Orbitron_Light_24.h>
#include <fonts/Orbitron_Med_12.h>
#include <fonts/Orbitron_Med_16.h>
#include <fonts/Orbitron_Bold_48.h>

#define FONT_SM &Orbitron_Medium_12
#define FONT_MED &Orbitron_Medium_16
#define FONT_LG &Orbitron_Light_24
#define FONT_XL &Orbitron_Light_32
#define FONT_XL_B &Orbitron_Bold_48

#define TOP_BAR 10
#define LINE_1 1
#define LINE_2 2
#define LINE_3 3
#define MARGIN 5

TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH); // Invoke custom library

enum FontSize
{
  SM,
  MED,
  LG,
  XL
};

#define TFT_H

enum Aligned
{
  ALIGNED_LEFT,
  ALIGNED_CENTRE,
  ALIGNED_RIGHT
};

enum MessageStatus
{
  OKAY = 0,
  WARNING,
  CRITICAL
};

uint16_t status_colours[3] = {TFT_BLUE, TFT_ORANGE, TFT_RED};

uint16_t calulateX(uint16_t x, uint16_t width, uint8_t datum);
uint16_t calculateY(uint16_t y, uint16_t height, uint8_t datum);

// TFT_eSprite _spr = TFT_eSprite(&tft);

// https://github.com/skelstar/esk8Project/blob/master/Controller/Display.h

void setupLCD()
{
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_DEFAULT_BG);
  tft.setTextColor(TFT_WHITE, TFT_DEFAULT_BG);
  tft.setTextSize(3);
}

//--------------------------------------------------------------------------------

int getX(Aligned aligned)
{
  switch (aligned)
  {
  case ALIGNED_LEFT:
    tft.setTextDatum(TL_DATUM);
    break;
  case ALIGNED_CENTRE:
    tft.setTextDatum(TC_DATUM);
    break;
  case ALIGNED_RIGHT:
  default:
    tft.setTextDatum(TR_DATUM);
    break;
  }
  return MARGIN;
}

int getY(uint8_t line)
{
  uint8_t line_height = (LCD_HEIGHT - TOP_BAR) / 3;
  return TOP_BAR + ((line - 1) * line_height);
}

void lcd_messageBase(const char *message, int y, Aligned aligned, FontSize size, uint32_t colour, uint32_t bgColour = TFT_DEFAULT_BG)
{
  uint8_t x = MARGIN;

  switch (size)
  {
  case FontSize::SM:
    tft.setFreeFont(FONT_SM);
    break;
  case FontSize::MED:
    tft.setFreeFont(FONT_MED);
    break;
  case FontSize::LG:
    tft.setFreeFont(FONT_LG);
    break;
  case FontSize::XL:
    tft.setFreeFont(FONT_XL);
    break;
  }

  tft.setTextColor(colour, bgColour);

  if (aligned == ALIGNED_LEFT)
  {
    tft.setTextDatum(TL_DATUM);
    tft.drawString(message, x, y);
  }
  else if (aligned == ALIGNED_RIGHT)
  {
    tft.setTextDatum(TR_DATUM);
    tft.drawString(message, LCD_WIDTH - MARGIN, y);
  }
  else if (aligned == ALIGNED_CENTRE)
  {
    tft.setTextDatum(TC_DATUM);
    tft.drawString(message, LCD_WIDTH / 2, y);
  }
}
//--------------------------------------------------------------------------------

void lcd_message(const char *message, uint8_t line, Aligned aligned, FontSize size = FontSize::MED, MessageStatus status = OKAY)
{
  uint8_t line_height = (LCD_HEIGHT - TOP_BAR) / 4;
  uint8_t y = TOP_BAR + ((line - 1) * line_height);

  uint32_t colour = TFT_WHITE;

  lcd_messageBase(message, y, aligned, size, colour, status == OKAY ? TFT_DEFAULT_BG : status_colours[status]);
}
//--------------------------------------------------------------------------------
void drawGraphFullWidth(uint8_t y, uint8_t height, float pc, uint16_t colour = TFT_WHITE)
{
  tft.fillRect(0, y, LCD_WIDTH, height, TFT_BLUE);
  tft.fillRect(0, y, (LCD_WIDTH * pc), height, colour);
}
//--------------------------------------------------------------------------------

void drawBattery(BatteryLib batt, uint8_t x1, uint8_t y1, uint8_t width, uint8_t height, uint32_t colour = TFT_WHITE)
{

  // #define BORDER_SIZE 2

  //   _spr.createSprite(/*width*/ width, /*height*/ height);

  //   uint8_t
  //       x = 0,
  //       y = 0,
  //       knobWidth = 4,
  //       knobHeight = 10,
  //       w = width - knobWidth,
  //       h = height;

  //   // knob
  //   _spr.fillRect(x, y + ((h - knobHeight) / 2), knobWidth, knobHeight, colour);

  //   // body - outline
  //   x += knobWidth;
  //   _spr.fillRect(x, y, w, h, colour);

  //   // body - empty inside
  //   x += BORDER_SIZE;
  //   y += BORDER_SIZE;
  //   w -= BORDER_SIZE * 2;
  //   h -= BORDER_SIZE * 2;
  //   _spr.fillRect(x, y, w, h, TFT_BLACK);

  //   // capacity
  //   w -= BORDER_SIZE * 2;
  //   h -= BORDER_SIZE * 2;

  //   x += BORDER_SIZE;
  //   y += BORDER_SIZE;
  //   _spr.fillRect(x, y, w, h, colour); // solid

  //   if (false == batt.isCharging)
  //   {
  //     // black rect for used part
  //     w = w * (1 - (batt.chargePercent / 100.0));
  //     _spr.fillRect(x, y, w, h, TFT_BLACK);
  //     DEBUGVAL(batt.chargePercent);
  //   }
  //   else
  //   {
  //     // plus
  //     const int plus = 4, edge = 3;
  //     _spr.fillRect(x + (w / 2) - (edge + (plus / 2)),
  //                   y + (h / 2) - (plus / 2),
  //                   edge + plus + edge,
  //                   plus,
  //                   TFT_BLACK);
  //     _spr.fillRect(x + (w / 2) - (plus / 2),
  //                   y + (h / 2) - (edge + (plus / 2)),
  //                   plus,
  //                   edge + plus + edge,
  //                   TFT_BLACK);
  //   }
  //   _spr.pushSprite(x1, y1);
}
//--------------------------------------------------------------------------------

void drawSmallBattery(uint8_t percent, uint16_t x, uint16_t y, uint16_t width, uint8_t datum, bool charging, uint32_t bgColour = TFT_BLACK)
{
  uint32_t colour = TFT_WHITE;
  uint16_t height = (0.45) * width,
           knobWidth = (0.1) * width;
  x = calulateX(x, width, datum);
  y = calculateY(y, height, datum);

  // body
  tft.fillRect(x + knobWidth, y, width - knobWidth, height, colour);
  // knob
  tft.fillRect(x, y + (height / 4), knobWidth, height - ((height / 4) * 2), colour);

  if (charging)
  {
    uint8_t halfx = x + ((knobWidth + width) / 2.0),
            halfy = y + height / 2,
            trixoffset = width / 9.0;
    tft.fillTriangle(halfx, y + 2, halfx - trixoffset, halfy, halfx, halfy, bgColour);
    tft.fillTriangle(halfx, halfy, halfx + trixoffset, halfy, halfx, y + height - 2, bgColour);
  }
  else
  {
    // capacity (remove from 100% using black box)
    uint8_t remove_box_width = ((100 - percent) / 100.0) * (width - knobWidth);
    tft.fillRect(x + knobWidth + 1, y + 1, remove_box_width, height - 2, bgColour);
  }
  // is charging
}

//--------------------------------------------------------------------------------
uint16_t calulateX(uint16_t x, uint16_t width, uint8_t datum)
{
  switch (datum)
  {
  case (uint8_t)TL_DATUM:
  case (uint8_t)ML_DATUM:
  case (uint8_t)BL_DATUM:
    return x;
    break;
  case (uint8_t)TC_DATUM:
  case (uint8_t)MC_DATUM:
  case (uint8_t)BC_DATUM:
    return x - (width / 2);
    break;
  case (uint8_t)TR_DATUM:
  case (uint8_t)MR_DATUM:
  case (uint8_t)BR_DATUM:
    return x - width;
    break;
  }
  return 0;
}
//--------------------------------------------------------------------------------
// returns the top of the object from y being the bottom/top/middle depending on datum
uint16_t calculateY(uint16_t y, uint16_t height, uint8_t datum)
{
  switch (datum)
  {
  case TL_DATUM:
  case TC_DATUM:
  case TR_DATUM:
    return y;
    break;
  case ML_DATUM:
  case MC_DATUM:
  case MR_DATUM:
    return y - (height / 2);
    break;
  case BL_DATUM:
  case BC_DATUM:
  case BR_DATUM:
    return y - height;
    break;
  }
  return 0;
}
//--------------------------------------------------------------------------------

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
//--------------------------------------------------------------------------------
