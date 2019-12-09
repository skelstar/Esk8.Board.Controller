// ST7789 135 x 240 display with no chip select line
#include <TFT_eSPI.h>
#include <SPI.h>

#define ST7789_DRIVER     // Configure all registers

#define TFT_WIDTH  240
#define TFT_HEIGHT 135

#define CGRAM_OFFSET      // Library will add offsets required


// SPRITES stuff
// https://github.com/skelstar/esk8Project/blob/5db5722ed70cbee6d732bda6a1be2c75ffc68f05/Controller/Controller.ino

// #define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue
//#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red

//#define TFT_INVERSION_ON
//#define TFT_INVERSION_OFF

// DSTIKE stepup
//#define TFT_DC    23
//#define TFT_RST   32
//#define TFT_MOSI  26
//#define TFT_SCLK  27

// Generic ESP32 setup
#define TFT_MISO -1
#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS    5 // Not connected
#define TFT_DC    16
#define TFT_RST   23  // Connect reset to ensure display initialises
#define TFT_BL 4
// #define TFT_BACKLIGHT_ON HIGH

// For NodeMCU - use pin numbers in the form PIN_Dx where Dx is the NodeMCU pin designation
// #define TFT_CS   -1      // Define as not used
// #define TFT_DC   PIN_D1  // Data Command control pin
//#define TFT_RST  PIN_D4  // TFT reset pin (could connect to NodeMCU RST, see next line)
// #define TFT_RST  -1      // TFT reset pin connect to NodeMCU RST, must also then add 10K pull down to TFT SCK


#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT


// #define SPI_FREQUENCY  27000000
#define SPI_FREQUENCY  40000000

#define SPI_READ_FREQUENCY  20000000

#define SPI_TOUCH_FREQUENCY  2500000

// #define SUPPORT_TRANSACTIONS

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

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

//--------------------------------------------------------------------------------

void chunky_draw_digit(
    uint8_t digit,
    uint8_t x,
    uint8_t y,
    uint16_t fg_color, 
    uint16_t bg_color, 
    uint8_t pixelSize = 1)
{

  for (int xx = 0; xx < 3; xx++)
  {
    for (int yy = 0; yy < 5; yy++)
    {
      uint16_t color = FONT_DIGITS_3x5[digit][yy][xx] ? fg_color : bg_color; 
      int x1 = x + xx * pixelSize;
      int y1 = y + yy * pixelSize;
      tft.fillRect(x1, y1, pixelSize, pixelSize, color);
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
      chunky_draw_digit(ch - '0', cursor_x, y, TFT_WHITE, TFT_BLACK, pixelSize);
      cursor_x += 3 * pixelSize + spacing;
    }
    else if (ch == '.')
    {
      tft.fillRect(cursor_x, y + 4 * pixelSize, pixelSize, pixelSize, TFT_WHITE);
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
      chunky_draw_digit(9 + 1, cursor_x, y, TFT_WHITE, TFT_BLACK, pixelSize);
      cursor_x += 3 * pixelSize + spacing;
    }
  }
  // units
  tft.drawString(units, cursor_x + spacing, y + 10);
}
//---------------------------------------------------------------
void display_initialise()
{
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("ready", TFT_WIDTH/2, TFT_HEIGHT/2, 2);
}
//---------------------------------------------------------------

void lcdMessage(char *message)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(message, TFT_WIDTH/2, TFT_HEIGHT/2, 2);
}
//---------------------------------------------------------------

void tft_util_draw_digit(
    TFT_eSprite *tft,
    uint8_t digit,
    uint8_t x,
    uint8_t y,
    uint16_t fg_color,
    uint16_t bg_color,
    uint8_t pixelSize = 1)
{

  for (int xx = 0; xx < 3; xx++)
  {
    for (int yy = 0; yy < 5; yy++)
    {
      uint16_t color = FONT_DIGITS_3x5[digit][yy][xx] ? fg_color : bg_color;
      int x1 = x + xx * pixelSize;
      int y1 = y + yy * pixelSize;
      tft->fillRect(x1, y1, pixelSize, pixelSize, color);
    }
  }
}




//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
//---------------------------------------------------------------
