#ifndef _TFT_eSPIH_
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
  enum ScreenLine
  {
    LINE1_OF_3,
    LINE2_OF_3,
    LINE3_OF_3,
    LINE1_OF_2,
    LINE2_OF_2,
  };

  ChunkyDigit(TFT_eSPI *tft, uint8_t pixel_size, uint8_t spacing, uint32_t bgColour)
  {
    _tft = tft;
    _pixel_size = pixel_size;
    _spacing = spacing;
    _bgColour = bgColour;
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
    return width;
  }

  //--------------------------------------------------------------------------------

  int getWidth(char *number, char *units = "")
  {
    bool unitsNotEmpty = units[0] != '\0';
    int unitsWidth = unitsNotEmpty
                         ? (int)_tft->textWidth(units)
                         : 0;
    return get_str_width(number) + unitsWidth;
  }

  void draw_float(uint8_t x, int y, char *number, char *units = "")
  {
    int number_len = strlen(number);
    bool unitsNotEmpty = units[0] != '\0';
    uint8_t cursor_x = x;

    for (int i = 0; i < number_len; i++)
    {
      char ch = number[i];
      uint8_t spc = i == number_len - 1 ? 0 : _spacing;
      if (ch >= '0' and ch <= '9')
      {
        chunky_draw_digit(ch - '0', cursor_x, y, _pixel_size);
        cursor_x += 3 * _pixel_size + spc;
      }
      else if (ch == '.')
      {
        _tft->fillRect(cursor_x, y + 4 * _pixel_size, _pixel_size, _pixel_size, TFT_WHITE);
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
    if (unitsNotEmpty)
    {
      int unitsY = y + (_pixel_size * 5 - _tft->fontHeight());
      _tft->drawString(units, cursor_x + 10, unitsY);
    }
  }

  void draw_float(uint8_t datum, char *number, char *units = "")
  {
    bool unitsNotEmpty = units[0] != '\0';
    int unitsWidth = unitsNotEmpty
                         ? (int)_tft->textWidth(units)
                         : 0;
    int width = get_str_width(number) + unitsWidth;

    draw_float(getX(datum, width), _getY(datum), number, units);
  }

  void draw_float(uint8_t datum, ScreenLine line, char *number, char *units)
  {
    bool unitsNotEmpty = units[0] != '\0';
    int unitsWidth = unitsNotEmpty
                         ? (int)_tft->textWidth(units)
                         : 0;
    int width = get_str_width(number) + unitsWidth;

    draw_float(getX(datum, width), _getY(line), number, units);
  }

  int getX(uint8_t datum, int width)
  {

    if (datum == TL_DATUM || datum == ML_DATUM || datum == BL_DATUM)
    {
      return 0;
    }
    else if (datum == TC_DATUM || datum == MC_DATUM || datum == BC_DATUM)
    {
      return (LCD_WIDTH / 2) - (width / 2);
    }
    return LCD_WIDTH - width;
  }

private:
  TFT_eSPI *_tft;
  uint32_t _bgColour;
  uint8_t _pixel_size, _spacing;

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
        uint32_t pixelColour = FONT_DIGITS_3x5[digit][yy][xx] ? TFT_WHITE : _bgColour;
        _tft->fillRect(x1, y1, _pixel_size, _pixel_size, pixelColour);
      }
    }
  }

  int _getY(ScreenLine line)
  /*
  returns the top of the line
  */
  {
    const uint8_t lines3margin = 3, lines2margin = 10;
    uint8_t digitHeight = _pixel_size * 5;
    switch (line)
    {
    case LINE1_OF_3:
      return lines3margin;
    case LINE2_OF_3:
      return LCD_HEIGHT / 2 - digitHeight / 2;
    case LINE3_OF_3:
      return LCD_HEIGHT - lines3margin - digitHeight;
    case LINE1_OF_2:
      return lines2margin;
      break;
    case LINE2_OF_2:
      return LCD_HEIGHT - lines2margin - digitHeight;
      break;
    }
  }

  int _getY(uint8_t datum)
  {
    uint8_t height = 5 * _pixel_size;

    if (datum == TL_DATUM || datum == TC_DATUM || datum == TR_DATUM)
    {
      return 0;
    }
    else if (datum == ML_DATUM || datum == MC_DATUM || datum == MR_DATUM)
    {
      return (LCD_HEIGHT / 2) - (height / 2);
    }
    return LCD_HEIGHT - height;
  }
};