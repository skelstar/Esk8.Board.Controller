
#ifndef TFT_eSPI
#include <TFT_eSPI.h>
#endif

#define TOP_BAR 10
#define LINE_1 1
#define LINE_2 2
#define LINE_3 3
#define MARGIN 5

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

uint16_t get_x_from_datum(uint16_t x, uint16_t width, uint8_t datum);
uint16_t get_y_from_datum(uint16_t y, uint16_t height, uint8_t datum);

// https://github.com/skelstar/esk8Project/blob/master/Controller/Display.h

void setupLCD()
{
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(3);
  tft.drawString("ready", 20, 20);
}

//--------------------------------------------------------------------------------
void lcd_message(const char *message, uint8_t line, Aligned aligned, MessageStatus status = OKAY)
{
  uint8_t line_height = (LCD_HEIGHT - TOP_BAR) / 3;
  uint8_t x = MARGIN,
          y = TOP_BAR + ((line - 1) * line_height);

  tft.setTextSize(3);
  if (status != OKAY)
  {
    tft.setTextColor(TFT_WHITE, status_colours[status]);
  }
  else
  {
    tft.setTextColor(TFT_WHITE);
  }

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
    tft.drawString(message, x, y);
  }
}
//--------------------------------------------------------------------------------
void drawGraphFullWidth(uint8_t y, uint8_t height, float pc, uint16_t colour = TFT_WHITE)
{
  tft.fillRect(0, y, LCD_WIDTH, height, TFT_BLUE);
  tft.fillRect(0, y, (LCD_WIDTH * pc), height, colour);
}
//--------------------------------------------------------------------------------

void drawBattery(int percent, bool update)
{
  // if (!update)
  // {
  //   return;
  // }

  // u8g2.clearBuffer();
  // int outsideX = (LCD_WIDTH - (BATTERY_WIDTH + BORDER_SIZE)) / 2; // includes batt knob
  // int outsideY = (LCD_HEIGHT - BATTERY_HEIGHT) / 2;
  // u8g2.drawBox(outsideX, outsideY, BATTERY_WIDTH, BATTERY_HEIGHT);
  // u8g2.drawBox(
  //     outsideX + BATTERY_WIDTH,
  //     outsideY + (BATTERY_HEIGHT - KNOB_HEIGHT) / 2,
  //     BORDER_SIZE,
  //     KNOB_HEIGHT); // knob
  // u8g2.setDrawColor(0);
  // u8g2.drawBox(
  //     outsideX + BORDER_SIZE,
  //     outsideY + BORDER_SIZE,
  //     BATTERY_WIDTH - BORDER_SIZE * 2,
  //     BATTERY_HEIGHT - BORDER_SIZE * 2);
  // u8g2.setDrawColor(1);
  // u8g2.drawBox(
  //     outsideX + BORDER_SIZE * 2,
  //     outsideY + BORDER_SIZE * 2,
  //     (BATTERY_WIDTH - BORDER_SIZE * 4) * percent / 100,
  //     BATTERY_HEIGHT - BORDER_SIZE * 4);
  // u8g2.sendBuffer();
}
//--------------------------------------------------------------------------------

#define SM_BATT_WIDTH 30
#define SM_BATT_HEIGHT 12
#define SM_BATT_KNOB_WIDTH 3

//--------------------------------------------------------------------------------
void drawSmallBattery(uint8_t percent, uint16_t x, uint16_t y, uint8_t datum)
{
  uint32_t colour = percent > 50 ? TFT_WHITE : percent > 30 ? TFT_YELLOW : TFT_ORANGE;

  x = get_x_from_datum(x, SM_BATT_WIDTH, datum);
  y = get_y_from_datum(y, SM_BATT_HEIGHT, datum);

  tft.fillRect(x + SM_BATT_KNOB_WIDTH, y, SM_BATT_WIDTH - SM_BATT_KNOB_WIDTH, SM_BATT_HEIGHT, colour);
  // knob
  tft.fillRect(x, y + 3, SM_BATT_KNOB_WIDTH, SM_BATT_HEIGHT - (3 * 2), colour);
  // capacity (remove from 100% using black box)
  uint8_t remove_box_width = ((100 - percent) / 100.0) * (SM_BATT_WIDTH - SM_BATT_KNOB_WIDTH);
  tft.fillRect(x + SM_BATT_KNOB_WIDTH + 1, y + 1, remove_box_width, SM_BATT_HEIGHT - 2, TFT_BLUE);
}
//--------------------------------------------------------------------------------
uint16_t get_x_from_datum(uint16_t x, uint16_t width, uint8_t datum)
{
  switch (datum)
  {
  case TL_DATUM:
  case ML_DATUM:
  case BL_DATUM:
    return x;
    break;
  case TC_DATUM:
  case MC_DATUM:
  case BC_DATUM:
    return x - (width / 2);
    break;
  case TR_DATUM:
  case MR_DATUM:
  case BR_DATUM:
    return x - width;
    break;
  }
  return 0;
}
//--------------------------------------------------------------------------------
// returns the top of the object from y being the bottom/top/middle depending on datum
uint16_t get_y_from_datum(uint16_t y, uint16_t height, uint8_t datum)
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
