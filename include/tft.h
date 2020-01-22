

enum Aligned
{
  ALIGNED_LEFT,
  ALIGNED_CENTRE,
  ALIGNED_RIGHT
};

//https://github.com/olikraus/u8g2/wiki/fntgrpiconic#open_iconic_arrow_2x2

#define LCD_WIDTH
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
// https://github.com/skelstar/esk8Project/blob/5db5722ed70cbee6d732bda6a1be2c75ffc68f05/Controller/Display.h


void setup_tft()
{
  // digitalWrite(TFT_CS, LOW);
  // digitalWrite(SPI_CS, HIGH);
  tft.init(135, 240);   // initialize a ST7789 chip, 240x240 pixels

  tft.setRotation(2);  
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); // Backlight on

  tft.setCursor(20, 20);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.print("ready!");
  DEBUG("setup_tft");
  // digitalWrite(SPI_CS, LOW);
  // digitalWrite(TFT_CS, HIGH);
}
//--------------------------------------------------------------------------------
void chunky_draw_digit(
    uint8_t digit,
    uint8_t x,
    uint8_t y,
    uint8_t pixelSize = 1)
{

  // for (int xx = 0; xx < 3; xx++)
  // {
  //   for (int yy = 0; yy < 5; yy++)
  //   {
  //     int x1 = x + xx * pixelSize;
  //     int y1 = y + yy * pixelSize;
  //     u8g2.setDrawColor(FONT_DIGITS_3x5[digit][yy][xx]);
  //     u8g2.drawBox(x1, y1, pixelSize, pixelSize);
  //   }
  // }
}
//--------------------------------------------------------------------------------
void chunkyDrawFloat(uint8_t x, uint8_t y, char *number, char *units, uint8_t spacing, uint8_t pixelSize = 1)
{

  // int cursor_x = x;
  // int number_len = strlen(number);

  // for (int i = 0; i < number_len; i++)
  // {
  //   char ch = number[i];
  //   if (ch >= '0' and ch <= '9')
  //   {
  //     chunky_draw_digit(ch - '0', cursor_x, y, pixelSize);
  //     cursor_x += 3 * pixelSize + spacing;
  //   }
  //   else if (ch == '.')
  //   {
  //     u8g2.drawBox(cursor_x, y + 4 * pixelSize, pixelSize, pixelSize);
  //     cursor_x += pixelSize + spacing;
  //   }
  //   else if (ch == '-')
  //   {
  //   }
  //   else if (ch == ' ')
  //   {
  //     cursor_x += 3 * pixelSize + spacing;
  //   }
  //   else if (ch == '%')
  //   {
  //     chunky_draw_digit(9 + 1, cursor_x, y, pixelSize);
  //     cursor_x += 3 * pixelSize + spacing;
  //   }
  // }
  // // units
  // u8g2.setFont(FONT_SIZE_MED_SMALL);
  // u8g2.drawStr(cursor_x + spacing, y + u8g2.getMaxCharHeight(), units);
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void clearScreen()
{
  // u8g2.clearBuffer();
  // u8g2.sendBuffer();
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// //--------------------------------------------------------------------------------
// void lcdPrimaryTemplate(
//     char *topLine,
//     char *primaryLine,
//     char *primaryUnits,
//     char *bottomLineLeft,
//     char *bottomLineRight,
//     bool warning)
// {
//   u8g2.clearBuffer();
//   // top line
//   if (warning)
//   {
//     u8g2.drawBox(0, 0, LCD_HEIGHT, FONT_SIZE_MED_SMALL_LINE_HEIGHT - 1);
//     u8g2.setDrawColor(0);
//   }
//   u8g2.setFontPosTop();
//   u8g2.setFont(FONT_SIZE_MED_SMALL);
//   int width = u8g2.getStrWidth(topLine);
//   u8g2.drawStr((LCD_HEIGHT - width) / 2, 0, topLine);
//   if (warning)
//   {
//     u8g2.setDrawColor(1);
//   }
//   // middle line
//   uint8_t pixelSize = 6;
//   uint8_t spacing = 4;
//   width = strlen(primaryLine) * 3 + (strlen(primaryLine) * (spacing - 1));
//   chunkyDrawFloat(0 + width / 2, LCD_WIDTH / 2 - (pixelSize * 5) / 2, primaryLine, primaryUnits, spacing, pixelSize);
//   // bottom line
//   u8g2.setFontPosBottom();
//   u8g2.setFont(FONT_SIZE_MED);
//   // left
//   u8g2.drawStr(0, LCD_WIDTH, bottomLineLeft);
//   // right
//   width = u8g2.getStrWidth(bottomLineRight);
//   u8g2.drawStr(LCD_HEIGHT - width, LCD_WIDTH, bottomLineRight);
//   // send
//   u8g2.sendBuffer();
// }
//--------------------------------------------------------------------------------
void lcd_message(char *message)
{
  // u8g2.clearBuffer();
  // u8g2.setFontPosCenter(); // vertical center
  // u8g2.setFont(u8g2_font_logisoso18_tr);
  // int width = u8g2.getStrWidth(message);
  // u8g2.drawStr(LCD_WIDTH / 2 - width / 2, LCD_HEIGHT / 2, message);
  // u8g2.sendBuffer();

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
    // u8g2.setFontPosTop(); // vertical center
    y = 0;
    break;
  case 2:
    // u8g2.setFontPosCenter(); // vertical center
    y = LCD_HEIGHT / 2;
    break;
  case 3:
    // u8g2.setFontPosBottom(); // vertical center
    y = LCD_HEIGHT;
    break;
  }
  // u8g2.setFont(u8g2_font_courB12_tr);
  // u8g2.drawStr(x, y, message);
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

#define SM_BATT_WIDTH 20
#define SM_BATT_HEIGHT 10

void draw_small_battery(uint8_t percent, uint8_t x, uint8_t y)
{
  // u8g2.setDrawColor(1);
  // u8g2.drawBox(x, y, SM_BATT_WIDTH, SM_BATT_HEIGHT);
  // // knob
  // u8g2.drawBox(x - 2, y + 3, 2, 4);
  // // capacity (remove from 100% using black box)
  // u8g2.setDrawColor(0);
  // uint8_t remove_box_width = ((100 - percent) / 100.0) * (SM_BATT_WIDTH - 2);
  // u8g2.drawBox(x + 1, y + 1, remove_box_width, SM_BATT_HEIGHT - 2);
  // u8g2.setDrawColor(1);
}
