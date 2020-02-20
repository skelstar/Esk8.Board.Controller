#ifndef TriggerState
#include <TriggerLib.h>
#endif

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

  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, status_colours[status]);
  if (aligned == ALIGNED_LEFT)
  {
    tft.drawString(message, x, y, TL_DATUM);
  }
  else if (aligned == ALIGNED_RIGHT)
  {
    tft.drawString(message, x, y, TR_DATUM);
  }
  else if (aligned == ALIGNED_CENTRE)
  {
    tft.drawString(message, x, y, TC_DATUM);
  }
}
//--------------------------------------------------------------------------------
void draw_fw_graph(uint8_t y, uint8_t height, float pc, uint16_t colour = TFT_WHITE)
{
  tft.fillRect(0, y, LCD_WIDTH, height, TFT_BLUE);
  tft.fillRect(0, y, (LCD_WIDTH * pc), height, colour);
}
//--------------------------------------------------------------------------------
void draw_response_graph()
{
  // float pc = stats.resp_time / 200.0;
  // draw_fw_graph(0, TOP_BAR, pc);
}
//--------------------------------------------------------------------------------
void draw_throttle(uint8_t throttle)
{
  if (throttle > 127)
  {
    float r = (throttle - 127.0) / 127.0;
    int w = (LCD_WIDTH / 2) * r;
    tft.fillRect(LCD_WIDTH / 2, 20, w, 5, TFT_GREEN);
  }
  else if (throttle < 127)
  {
    float r = (127.0 - throttle) / 127.0;
    int w = (LCD_WIDTH / 2) * r;
    tft.fillRect((LCD_WIDTH / 2) - w, 20, w, 5, TFT_RED);
  }
  else
  {
    tft.fillRect(0, 20, LCD_WIDTH, 5, TFT_BLUE);
    // tft.drawLine(LCD_WIDTH/2, 20, LCD_WIDTH/2+1, 25, TFT_WHITE);
  }
}
//--------------------------------------------------------------------------------

#define BATTERY_WIDTH 100
#define BATTERY_HEIGHT 50
#define BORDER_SIZE 6
#define KNOB_HEIGHT 20

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
void draw_small_battery(uint8_t percent, uint16_t x, uint16_t y, uint8_t datum)
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
void draw_trigger_state(TriggerState state, uint8_t datum)
{
  // uint8_t width = 20, height = 20;
  // uint8_t x = get_x(datum, width);
  // uint8_t y = get_y_and_set_pos(datum, height);
  // uint8_t x2 = x + width/2 - 5;
  // uint8_t y2 = y + 17;

  // if (state == IDLE_STATE || state == GO_STATE)
  // {
  //   u8g2.drawFrame(x, y, width, height);
  //   u8g2.drawStr(x2, y2, state == IDLE_STATE ? "I" : "G");
  // }
  // else
  // {
  //   u8g2.drawBox(x, y, width, height);
  //   u8g2.setDrawColor(0);
  //   u8g2.drawStr(x2, y2, "W");
  //   u8g2.setDrawColor(1);
  // }
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

/// returns the top of the object from y being the bottom/top/middle depending on datum
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
}
// //--------------------------------------------------------------------------------
// char *getIntString(char *buff, int val)
// {
//   itoa(val, buff, 10);
//   return buff;
// }
// //--------------------------------------------------------------------------------
// char *getFloatString(char *buff, float val, uint8_t upper, uint8_t lower)
// {
//   dtostrf(val, upper, lower, buff);
//   return buff;
// }
// //--------------------------------------------------------------------------------
// char *getParamFloatString(char *buff, float val, uint8_t upper, uint8_t lower, char *param)
// {
//   dtostrf(val, upper, lower, buff);
//   sprintf(buff, param, buff);
//   return buff;
// }
//--------------------------------------------------------------------------------
