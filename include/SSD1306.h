#ifndef Wire
#include <Wire.h>
#endif

#include <U8g2lib.h>

#define USING_SSD1306 1

enum Aligned
{
  ALIGNED_LEFT,
  ALIGNED_CENTRE,
  ALIGNED_RIGHT
};

//https://github.com/olikraus/u8g2/wiki/fntgrpiconic#open_iconic_arrow_2x2

#define OLED_SCL 15
#define OLED_SDA 4
#define OLED_RST 16
#define OLED_ADDR 0x3C
#define OLED_CONTRAST_HIGH 100 // 256 highest
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, /* clock=*/OLED_SCL, /* data=*/OLED_SDA, /* reset=*/OLED_RST);
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

#define LCD_WIDTH 128
#define LCD_HEIGHT 64
// u8g2.setFont(u8g2_font_tenfatguys_tf);
// u8g2.setFont(u8g2_font_tenthinguys_tf);

// u8g2_font_profont10_tr
// u8g2_font_profont11_tr
// u8g2_font_profont12_tr
// u8g2_font_profont15_tr
// u8g2_font_profont17_tr
// u8g2_font_profont22_tr
// u8g2_font_profont29_tr
#define FONT_SIZE_XSMALL u8g2_font_profont10_tr

#define FONT_SIZE_SMALL u8g2_font_profont12_tr

#define FONT_SIZE_MED_SMALL u8g2_font_profont15_tr
#define FONT_SIZE_MED_SMALL_LINE_HEIGHT 15

#define FONT_SIZE_MED u8g2_font_profont17_tr
#define FONT_SIZE_MED_LINE_HEIGHT 17
#define FONT_SIZE_MED_LINE_1 0
#define FONT_SIZE_MED_LINE_2 FONT_SIZE_MED_LINE_HEIGHT
#define FONT_SIZE_MED_LINE_3 FONT_SIZE_MED_LINE_HEIGHT * 2
#define FONT_SIZE_MED_LINE_4 FONT_SIZE_MED_LINE_HEIGHT * 3

#define BETWEEN_LINE1_AND_LINE2 LCD_WIDTH / 4
#define BETWEEN_LINE2_AND_LINE3 (LCD_WIDTH / 4) * 2

#define FONT_SIZE_LG u8g2_font_profont29_tr
#define FONT_SIZE_LG_LINE_HEIGHT 29
#define FONT_SIZE_LG_ALL (LCD_WIDTH / 2) - FONT_SIZE_LG_LINE_HEIGHT / 2
#define FONT_SIZE_LG_LINE_1 0
#define FONT_SIZE_LG_LINE_2 FONT_SIZE_MED_LINE_HEIGHT

// https://github.com/skelstar/esk8Project/blob/master/Controller/Display.h
const bool FONT_DIGITS_3x5[11][5][3] = {
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    },
    // % = 10
    {
        {1, 0, 1},
        {0, 0, 1},
        {0, 1, 0},
        {1, 0, 0},
        {1, 0, 1},
    }};
//--------------------------------------------------------------------------------
void setupLCD()
{
  u8g2.begin();
  u8g2.setContrast(OLED_CONTRAST_HIGH);
  u8g2.clearBuffer();
}
//--------------------------------------------------------------------------------
void chunky_draw_digit(
    uint8_t digit,
    uint8_t x,
    uint8_t y,
    uint8_t pixelSize = 1)
{

  for (int xx = 0; xx < 3; xx++)
  {
    for (int yy = 0; yy < 5; yy++)
    {
      int x1 = x + xx * pixelSize;
      int y1 = y + yy * pixelSize;
      u8g2.setDrawColor(FONT_DIGITS_3x5[digit][yy][xx]);
      u8g2.drawBox(x1, y1, pixelSize, pixelSize);
    }
  }
}
//--------------------------------------------------------------------------------
void chunkyDrawFloat(uint8_t x, uint8_t y, char *number, char *units, uint8_t spacing, uint8_t pixelSize = 1)
{

  int cursor_x = x;
  int number_len = strlen(number);

  for (int i = 0; i < number_len; i++)
  {
    char ch = number[i];
    if (ch >= '0' and ch <= '9')
    {
      chunky_draw_digit(ch - '0', cursor_x, y, pixelSize);
      cursor_x += 3 * pixelSize + spacing;
    }
    else if (ch == '.')
    {
      u8g2.drawBox(cursor_x, y + 4 * pixelSize, pixelSize, pixelSize);
      cursor_x += pixelSize + spacing;
    }
    else if (ch == '-')
    {
    }
    else if (ch == ' ')
    {
      cursor_x += 3 * pixelSize + spacing;
    }
    else if (ch == '%')
    {
      chunky_draw_digit(9 + 1, cursor_x, y, pixelSize);
      cursor_x += 3 * pixelSize + spacing;
    }
  }
  // units
  u8g2.setFont(FONT_SIZE_MED_SMALL);
  u8g2.drawStr(cursor_x + spacing, y + u8g2.getMaxCharHeight(), units);
}
//--------------------------------------------------------------------------------
#define BAR_GRAPH_THICKNESS 5
void lcdBarGraph(float percentage)
{
  u8g2.setDrawColor(1);
  float x2 = LCD_HEIGHT * percentage;
  u8g2.drawBox(0, LCD_WIDTH - BAR_GRAPH_THICKNESS, x2, BAR_GRAPH_THICKNESS);
}
//--------------------------------------------------------------------------------
void clearScreen()
{
  u8g2.clearBuffer();
  u8g2.sendBuffer();
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
  u8g2.setFontPosTop();
  char buff2[8];
  char buff[8]; // Buffer big enough for 7-character float
  dtostrf(value, 5, 1, buff);
  sprintf(buff2, paramtext, buff);
  u8g2.setFont(FONT_SIZE_MED); // full
  int width = u8g2.getStrWidth(buff2);
  u8g2.drawStr(x, y, label);
  u8g2.drawStr(LCD_HEIGHT - width, y, buff2);
}
//--------------------------------------------------------------------------------
void lcd_paramText(uint8_t x, uint8_t y, char *paramtext, float val1, float val2)
{
  u8g2.setFontPosTop();
  char buffx[8];
  char buff1[8]; // Buffer big enough for 7-character float
  char buff2[8]; // Buffer big enough for 7-character float
  sprintf(buffx, paramtext, getFloatString(buff1, val1, 4, 0), getFloatString(buff2, val2, 2, 1));
  u8g2.setFont(FONT_SIZE_MED); // full
  int width = u8g2.getStrWidth(buffx);
  u8g2.drawStr(x, y, buffx);
}
//--------------------------------------------------------------------------------
void lcd_line_text(uint8_t x, uint8_t y, char *text, bool vertical_centered, bool horizontal_centered)
{
  if (vertical_centered)
  {
    u8g2.setFontPosCenter();
  }
  else
  {
    u8g2.setFontPosTop();
  }
  u8g2.setFont(FONT_SIZE_MED); // full
  if (horizontal_centered)
  {
    int width = u8g2.getStrWidth(text);
    x = u8g2.getWidth() / 2 - width / 2;
  }
  u8g2.drawStr(x, y, text);
}
//--------------------------------------------------------------------------------
void lcdPrimaryTemplate(
    char *topLine,
    char *primaryLine,
    char *primaryUnits,
    char *bottomLineLeft,
    char *bottomLineRight,
    bool warning)
{
  u8g2.clearBuffer();
  // top line
  if (warning)
  {
    u8g2.drawBox(0, 0, LCD_HEIGHT, FONT_SIZE_MED_SMALL_LINE_HEIGHT - 1);
    u8g2.setDrawColor(0);
  }
  u8g2.setFontPosTop();
  u8g2.setFont(FONT_SIZE_MED_SMALL);
  int width = u8g2.getStrWidth(topLine);
  u8g2.drawStr((LCD_HEIGHT - width) / 2, 0, topLine);
  if (warning)
  {
    u8g2.setDrawColor(1);
  }
  // middle line
  uint8_t pixelSize = 6;
  uint8_t spacing = 4;
  width = strlen(primaryLine) * 3 + (strlen(primaryLine) * (spacing - 1));
  chunkyDrawFloat(0 + width / 2, LCD_WIDTH / 2 - (pixelSize * 5) / 2, primaryLine, primaryUnits, spacing, pixelSize);
  // bottom line
  u8g2.setFontPosBottom();
  u8g2.setFont(FONT_SIZE_MED);
  // left
  u8g2.drawStr(0, LCD_WIDTH, bottomLineLeft);
  // right
  width = u8g2.getStrWidth(bottomLineRight);
  u8g2.drawStr(LCD_HEIGHT - width, LCD_WIDTH, bottomLineRight);
  // send
  u8g2.sendBuffer();
}
//--------------------------------------------------------------------------------
void lcdConnectingPage(char *message, float ampHours, float odo)
{
  char avgbuff[8];
  char aHbuff[8];
  char kmBuff[8];
  float avgAh = ampHours > 0.0 && odo > 0.0
                    ? ampHours / odo
                    : 0.0;

  lcdPrimaryTemplate(
      "connecting...",
      /*primary*/ getFloatString(avgbuff, avgAh, 3, 0),
      /*primaryunits*/ "mAh/km",
      /*left*/ getParamFloatString(aHbuff, ampHours, 3, 0, "%smAh"),
      /*right*/ getParamFloatString(kmBuff, odo, 2, 1, "%skm"),
      /*warning*/ true);
}
//--------------------------------------------------------------------------------
void lcdTripPage(float ampHours, float odo, bool vescOnline, bool update)
{
  if (!update)
  {
    return;
  }
  u8g2.clearBuffer();
  char avgbuff[8];
  char aHbuff[8];
  char kmBuff[8];
  char titleBuff[16];
  float avgAh = ampHours > 0.0 && odo > 0.0 ? ampHours / odo : 0.0;

  strcpy(titleBuff, vescOnline
                        // 01234567890123456
                        ? "Trip"
                        : "VESC OFFLINE!!!");

  lcdPrimaryTemplate(
      /*title*/ titleBuff,
      /*primary*/ getFloatString(avgbuff, avgAh, 3, 0),
      /*primaryunits*/ "mAh/km",
      /*left*/ getParamFloatString(aHbuff, ampHours, 3, 0, "%smAh"),
      /*right*/ getParamFloatString(kmBuff, odo, 2, 1, "%skm"),
      /*warning*/ vescOnline == false);
  u8g2.sendBuffer();
}
//--------------------------------------------------------------------------------
void lcd_message(char *message)
{
  // vTaskSuspendAll();

  u8g2.clearBuffer();
  u8g2.setFontPosCenter(); // vertical center
  u8g2.setFont(u8g2_font_logisoso18_tr);
  int width = u8g2.getStrWidth(message);
  u8g2.drawStr(LCD_WIDTH / 2 - width / 2, LCD_HEIGHT / 2, message);
  u8g2.sendBuffer();

  // xTaskResumeAll();
}
//--------------------------------------------------------------------------------
void lcd_message(uint8_t line_number, const char *message, Aligned aligned = ALIGNED_CENTRE)
{
  uint8_t x = 0, y = 0;

  // int width = u8g2.getStrWidth(message);

  // switch (aligned)
  // {
  // ALIGNED_LEFT:
  //   x = 0;
  //   break;
  // ALIGNED_CENTRE:
  //   x = LCD_WIDTH / 2;
  //   break;
  // ALIGNED_RIGHT:
  //   x = LCD_WIDTH - width;
  //   break;
  // }

  switch (line_number)
  {
  case 1:
    u8g2.setFontPosTop(); // vertical center
    y = 0;
    break;
  case 2:
    u8g2.setFontPosCenter(); // vertical center
    y = LCD_HEIGHT / 2;
    break;
  case 3:
    u8g2.setFontPosBottom(); // vertical center
    y = LCD_HEIGHT;
    break;
  }
  u8g2.setFont(u8g2_font_courB12_tr);
  u8g2.drawStr(x, y, message);
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

  u8g2.clearBuffer();
  int outsideX = (LCD_WIDTH - (BATTERY_WIDTH + BORDER_SIZE)) / 2; // includes batt knob
  int outsideY = (LCD_HEIGHT - BATTERY_HEIGHT) / 2;
  u8g2.drawBox(outsideX, outsideY, BATTERY_WIDTH, BATTERY_HEIGHT);
  u8g2.drawBox(
      outsideX + BATTERY_WIDTH,
      outsideY + (BATTERY_HEIGHT - KNOB_HEIGHT) / 2,
      BORDER_SIZE,
      KNOB_HEIGHT); // knob
  u8g2.setDrawColor(0);
  u8g2.drawBox(
      outsideX + BORDER_SIZE,
      outsideY + BORDER_SIZE,
      BATTERY_WIDTH - BORDER_SIZE * 2,
      BATTERY_HEIGHT - BORDER_SIZE * 2);
  u8g2.setDrawColor(1);
  u8g2.drawBox(
      outsideX + BORDER_SIZE * 2,
      outsideY + BORDER_SIZE * 2,
      (BATTERY_WIDTH - BORDER_SIZE * 4) * percent / 100,
      BATTERY_HEIGHT - BORDER_SIZE * 4);
  u8g2.sendBuffer();
}
//--------------------------------------------------------------------------------

#define SM_BATT_WIDTH 20
#define SM_BATT_HEIGHT 10

void draw_small_battery(uint8_t percent, uint8_t x, uint8_t y)
{
  u8g2.setDrawColor(1);
  u8g2.drawBox(x, y, SM_BATT_WIDTH, SM_BATT_HEIGHT);
  // knob
  u8g2.drawBox(x - 2, y + 3, 2, 4);
  // capacity (remove from 100% using black box)
  u8g2.setDrawColor(0);
  uint8_t remove_box_width = ((100 - percent) / 100.0) * (SM_BATT_WIDTH - 2);
  u8g2.drawBox(x + 1, y + 1, remove_box_width, SM_BATT_HEIGHT - 2);
  u8g2.setDrawColor(1);
}
