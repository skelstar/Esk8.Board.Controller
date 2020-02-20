// #ifndef Wire
// #include <Wire.h>
// #endif

#ifndef TriggerState
#include <TriggerLib.h>
#endif

#ifndef Arduino
#include <Arduino.h>
#endif

#define TFT_H

// #include <U8g2lib.h>

// #define USING_SSD1306 1

enum Aligned
{
  ALIGNED_LEFT,
  ALIGNED_CENTRE,
  ALIGNED_RIGHT
};

// #define LCD_WIDTH 128
// #define LCD_HEIGHT 64

//--------------------------------------------------------------------------------
void setupLCD()
{
  //u8g2.begin();
  //u8g2.setContrast(OLED_CONTRAST_HIGH);
  //u8g2.clearBuffer();
}
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
#define BAR_GRAPH_THICKNESS 5
void lcdBarGraph(float percentage)
{
  //u8g2.setDrawColor(1);
  float x2 = LCD_HEIGHT * percentage;
  //u8g2.drawBox(0, LCD_WIDTH - BAR_GRAPH_THICKNESS, x2, BAR_GRAPH_THICKNESS);
}
//--------------------------------------------------------------------------------
void clearScreen()
{
  //u8g2.clearBuffer();
  //u8g2.sendBuffer();
}
//--------------------------------------------------------------------------------
char *getIntString(char *buff, int val)
{
  itoa(val, buff, 10);
  return buff;
}
//--------------------------------------------------------------------------------
char *getFloatString(char *buff, float val, uint8_t upper, uint8_t lower)
{
  dtostrf(val, upper, lower, buff);
  return buff;
}
//--------------------------------------------------------------------------------
char *getParamFloatString(char *buff, float val, uint8_t upper, uint8_t lower, char *param)
{
  dtostrf(val, upper, lower, buff);
  sprintf(buff, param, buff);
  return buff;
}
//--------------------------------------------------------------------------------
void lcd_medium_float_text(
    uint8_t x,
    uint8_t y,
    char *label,
    char *paramtext,
    float value)
{
  //u8g2.setFontPosTop();
  char buff2[8];
  char buff[8]; // Buffer big enough for 7-character float
  dtostrf(value, 5, 1, buff);
  sprintf(buff2, paramtext, buff);
  //u8g2.setFont(FONT_SIZE_MED); // full
  // int width = u8g2.getStrWidth(buff2);
  //u8g2.drawStr(x, y, label);
  //u8g2.drawStr(LCD_HEIGHT - width, y, buff2);
}
//--------------------------------------------------------------------------------
void lcd_message(char *message)
{
  // vTaskSuspendAll();

  //u8g2.clearBuffer();
  //u8g2.setFontPosCenter(); // vertical center
  //u8g2.setFont(u8g2_font_logisoso18_tr);
  // int width = u8g2.getStrWidth(message);
  //u8g2.drawStr(LCD_WIDTH / 2 - width / 2, LCD_HEIGHT / 2, message);
  //u8g2.sendBuffer();
}
//--------------------------------------------------------------------------------
void lcd_message(uint8_t line_number, const char *message, Aligned aligned = ALIGNED_CENTRE)
{
  uint8_t x = 0, y = 0;

  switch (line_number)
  {
  case 1:
    //u8g2.setFontPosTop(); // vertical center
    y = 0;
    break;
  case 2:
    //u8g2.setFontPosCenter(); // vertical center
    y = LCD_HEIGHT / 2;
    break;
  case 3:
    //u8g2.setFontPosBottom(); // vertical center
    y = LCD_HEIGHT;
    break;
  }
  //u8g2.setFont(u8g2_font_courB12_tr);
  //u8g2.drawStr(x, y, message);
}
//--------------------------------------------------------------------------------
uint8_t get_y_and_set_pos(uint8_t datum, uint8_t height)
{
  // if (datum == TL_DATUM || datum == TC_DATUM || datum == TR_DATUM)
  // {
  //   //u8g2.setFontPosTop();
  //   return 0;
  // }
  // else if (datum == ML_DATUM || datum == MC_DATUM || datum == MR_DATUM)
  // {
  //   //u8g2.setFontPosCenter();
  //   return LCD_HEIGHT / 2;
  // }
  // else
  // { // BL_DATUM, BC_DATUM, BR_DATUM:
  //   //u8g2.setFontPosBottom();
  //   return LCD_HEIGHT - height;
  // }
  return 0;
}
//--------------------------------------------------------------------------------
uint8_t get_x(uint8_t datum, uint8_t width)
{
  // if (datum == TL_DATUM || datum == ML_DATUM || datum == BL_DATUM)
  // {
  //   return 0;
  // }
  // else if (datum == TC_DATUM || datum == MC_DATUM || datum == BC_DATUM)
  // {
  //   return (LCD_WIDTH / 2) - (width / 2);
  // }
  // else if (datum == TR_DATUM || datum == MR_DATUM || datum == BR_DATUM)
  // {
  //   return LCD_WIDTH - width;
  // }
  return 0;
}
//--------------------------------------------------------------------------------
void lcd_message(const char *message, uint8_t datum)
{
  //u8g2.setFont(u8g2_font_courB12_tr);
  uint8_t x = 0, y = 0;
  // uint8_t width = u8g2.getStrWidth(message);
  //     x = get_x(datum, width);
  // y = get_y_and_set_pos(datum, u8g2.getMaxCharHeight());
  //u8g2.drawStr(x, y, message);
  //u8g2.drawStr(x, y, message);
}
//--------------------------------------------------------------------------------

#define BATTERY_WIDTH 100
#define BATTERY_HEIGHT 50
#define BORDER_SIZE 6
#define KNOB_HEIGHT 20

void drawBattery(int percent, bool update)
{
  if (!update)
  {
    return;
  }

  //u8g2.clearBuffer();
  int outsideX = (LCD_WIDTH - (BATTERY_WIDTH + BORDER_SIZE)) / 2; // includes batt knob
  int outsideY = (LCD_HEIGHT - BATTERY_HEIGHT) / 2;
  //u8g2.drawBox(outsideX, outsideY, BATTERY_WIDTH, BATTERY_HEIGHT);
  //u8g2.drawBox(
  // outsideX + BATTERY_WIDTH,
  // outsideY + (BATTERY_HEIGHT - KNOB_HEIGHT) / 2,
  // BORDER_SIZE,
  // KNOB_HEIGHT); // knob
  //u8g2.setDrawColor(0);
  //u8g2.drawBox(
  // outsideX + BORDER_SIZE,
  // outsideY + BORDER_SIZE,
  // BATTERY_WIDTH - BORDER_SIZE * 2,
  // BATTERY_HEIGHT - BORDER_SIZE * 2);
  //u8g2.setDrawColor(1);
  //u8g2.drawBox(
  // outsideX + BORDER_SIZE * 2,
  // outsideY + BORDER_SIZE * 2,
  // (BATTERY_WIDTH - BORDER_SIZE * 4) * percent / 100,
  // BATTERY_HEIGHT - BORDER_SIZE * 4);
  //u8g2.sendBuffer();
}
//--------------------------------------------------------------------------------

#define SM_BATT_WIDTH 20
#define SM_BATT_HEIGHT 10

void draw_small_battery(uint8_t percent, uint8_t x, uint8_t y)
{
  //u8g2.setDrawColor(1);
  //u8g2.drawBox(x, y, SM_BATT_WIDTH, SM_BATT_HEIGHT);
  // knob
  //u8g2.drawBox(x - 2, y + 3, 2, 4);
  // capacity (remove from 100% using black box)
  //u8g2.setDrawColor(0);
  uint8_t remove_box_width = ((100 - percent) / 100.0) * (SM_BATT_WIDTH - 2);
  //u8g2.drawBox(x + 1, y + 1, remove_box_width, SM_BATT_HEIGHT - 2);
  //u8g2.setDrawColor(1);
}
//--------------------------------------------------------------------------------

void draw_trigger_state(TriggerState state, uint8_t datum)
{
  uint8_t width = 20, height = 20;
  uint8_t x = get_x(datum, width);
  uint8_t y = get_y_and_set_pos(datum, height);
  uint8_t x2 = x + width / 2 - 5;
  uint8_t y2 = y + 17;

  if (state == IDLE_STATE || state == GO_STATE)
  {
    //u8g2.drawFrame(x, y, width, height);
    //u8g2.drawStr(x2, y2, state == IDLE_STATE ? "I" : "G");
  }
  else
  {
    //u8g2.drawBox(x, y, width, height);
    //u8g2.setDrawColor(0);
    //u8g2.drawStr(x2, y2, "W");
    //u8g2.setDrawColor(1);
  }
}
//--------------------------------------------------------------------------------
