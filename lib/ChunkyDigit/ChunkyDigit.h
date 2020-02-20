#ifndef TFT_eSPI
#include <TFT_eSPI.h>
#endif

class ChunkyDigit
{
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

public:
  ChunkyDigit(TFT_eSPI *tft, uint8_t pixel_size, uint8_t spacing)
  {
    _tft = tft;
    _pixel_size = pixel_size;
    _spacing = spacing;
  }
  //--------------------------------------------------------------------------------
  int get_str_width(char *number)
  {
    uint8_t num_chars = strlen(number);
    uint8_t char_width = 3 * _pixel_size;
    uint8_t width = 0;
    for (int i = 0; i < num_chars; i++)
    {
      uint8_t w = number[i] == '.' ? 1 * _pixel_size : char_width;
      if (i < num_chars - 1)
        w += _spacing;
      width += w;
    }
    return width; // (num_chars * char_width) + _pixel_size + ((num_chars - 1) * _spacing);
  }
  //--------------------------------------------------------------------------------
  void draw_float(uint8_t datum, char *number, char *units)
  {
    int number_len = strlen(number);
    int width = get_str_width(number);

    int cursor_x = datum == TL_DATUM || datum == ML_DATUM || datum == BL_DATUM
                       ? 0
                       : datum == TC_DATUM || datum == MC_DATUM || datum == BC_DATUM
                             ? (LCD_WIDTH / 2) - (width / 2)
                             : LCD_WIDTH - width;
    int y = datum == TL_DATUM || datum == TC_DATUM || datum == TR_DATUM
                ? 0
                : datum == ML_DATUM || datum == MC_DATUM || datum == MR_DATUM
                      ? LCD_HEIGHT / 2
                      : LCD_HEIGHT - (_pixel_size * 5);

    for (int i = 0; i < number_len; i++)
    {
      char ch = number[i];
      uint8_t spc = i == number_len - 1 ? 0 : _spacing;
      if (ch >= '0' and ch <= '9')
      {
        DEBUGVAL(cursor_x, spc, width);
        chunky_draw_digit(ch - '0', cursor_x, y, _pixel_size);
        cursor_x += 3 * _pixel_size + spc;
      }
      else if (ch == '.')
      {
        //_u8g2->drawBox(cursor_x, y + 4 * _pixel_size, _pixel_size, _pixel_size);
        cursor_x += _pixel_size + spc;
      }
      else if (ch == '-')
      {
      }
      else if (ch == ' ')
      {
        cursor_x += 3 * _pixel_size + spc;
      }
      else if (ch == '%')
      {
        chunky_draw_digit(9 + 1, cursor_x, y, _pixel_size);
        cursor_x += (3 * _pixel_size) + spc;
      }
    }
    // units
    if (units != NULL)
    {
      // //_u8g2->setFont(u8g2_font_profont15_tr);
      // //_u8g2->drawStr(cursor_x + _spacing, y + //_u8g2->getMaxCharHeight(), units);
    }
  }

private:
  // U8G2_SSD1306_128X64_NONAME_F_SW_I2C *//_u8g2;
  TFT_eSPI *_tft;
  uint8_t _pixel_size;
  uint8_t _spacing;

  void chunky_draw_digit(
      uint8_t digit,
      uint8_t x,
      uint8_t y,
      uint8_t _pixel_size = 1)
  {
    for (int xx = 0; xx < 3; xx++)
    {
      for (int yy = 0; yy < 5; yy++)
      {
        int x1 = x + xx * _pixel_size;
        int y1 = y + yy * _pixel_size;
        // //_u8g2->setDrawColor(FONT_DIGITS_3x5[digit][yy][xx]);
        // //_u8g2->drawBox(x1, y1, _pixel_size, _pixel_size);
      }
    }
  }
};