#ifndef _TFT_eSPIH_
#include <TFT_eSPI.h>
#endif

class SlantyDigit
{
  enum PixelType
  {
    _E = 0,
    XX,
    TL,
    TR,
    BL,
    Br,
  };

#define NUM_PIXELS_HIGH 5
#define NUM_PIXELS_WIDE 5

  const PixelType FONT_DIGITS_4x5[11 + 3 + 1][NUM_PIXELS_HIGH][NUM_PIXELS_WIDE] = {
      {
          // 0
          {TL, XX, XX, XX, TR},
          {XX, _E, _E, _E, XX},
          {XX, _E, XX, _E, XX},
          {XX, _E, _E, _E, XX},
          {BL, XX, XX, XX, Br},
      },
      {
          // 1
          {_E, _E, _E, _E, TL},
          {_E, _E, _E, _E, XX},
          {_E, _E, _E, _E, XX},
          {_E, _E, _E, _E, XX},
          {_E, _E, _E, _E, XX},
      },
      {
          // 2
          {TL, XX, XX, XX, TR},
          {_E, _E, _E, _E, XX},
          {TL, XX, XX, XX, Br},
          {XX, _E, _E, _E, _E},
          {XX, XX, XX, XX, XX},
      },
      {
          //3
          {TL, XX, XX, XX, TR},
          {_E, _E, _E, _E, XX},
          {_E, XX, XX, XX, XX},
          {_E, _E, _E, _E, XX},
          {BL, XX, XX, XX, Br},
      },
      {
          // 4
          {XX, _E, _E, _E, XX},
          {XX, _E, _E, _E, XX},
          {BL, XX, XX, XX, XX},
          {_E, _E, _E, _E, XX},
          {_E, _E, _E, _E, Br},
      },
      {
          // 5
          {XX, XX, XX, XX, XX},
          {XX, _E, _E, _E, _E},
          {XX, XX, XX, XX, TR},
          {_E, _E, _E, _E, XX},
          {XX, XX, XX, XX, Br},
      },
      {
          // 6
          {TL, XX, XX, XX, TR},
          {XX, _E, _E, _E, _E},
          {XX, XX, XX, XX, TR},
          {XX, _E, _E, _E, XX},
          {BL, XX, XX, XX, Br},
      },
      {
          // 7
          {XX, XX, XX, XX, XX},
          {_E, _E, _E, _E, XX},
          {_E, _E, _E, _E, XX},
          {_E, _E, _E, _E, XX},
          {_E, _E, _E, _E, Br},
      },
      {
          // 8
          {TL, XX, XX, XX, TR},
          {XX, _E, _E, _E, XX},
          {XX, XX, XX, XX, XX},
          {XX, _E, _E, _E, XX},
          {BL, XX, XX, XX, Br},
      },
      {
          // 9
          {TL, XX, XX, XX, TR},
          {XX, _E, _E, _E, XX},
          {BL, XX, XX, XX, XX},
          {_E, _E, _E, _E, XX},
          {BL, XX, XX, XX, Br},
      },
      {
          // % = 10
          {XX, _E, _E, _E, XX},
          {_E, _E, _E, XX, XX},
          {_E, _E, XX, _E, _E},
          {_E, XX, _E, _E, _E},
          {XX, _E, _E, _E, XX},
      },
      {
          // O
          {TL, XX, XX, XX, TR},
          {XX, _E, _E, _E, XX},
          {XX, _E, XX, _E, XX},
          {XX, _E, _E, _E, XX},
          {BL, XX, XX, XX, Br},
      },
      {
          // F
          {XX, XX, XX, XX, TR},
          {XX, _E, _E, _E, _E},
          {XX, XX, XX, XX, _E},
          {XX, _E, _E, _E, _E},
          {XX, _E, _E, _E, _E},
      },
      {
          // N
          {XX, XX, XX, XX, TR},
          {XX, _E, _E, _E, XX},
          {XX, _E, _E, _E, XX},
          {XX, _E, _E, _E, XX},
          {XX, _E, _E, _E, XX},
      },
      {
          // MISSING CHAR
          {_E, _E, _E, _E, _E},
          {_E, _E, _E, _E, _E},
          {XX, XX, XX, XX, XX},
          {_E, _E, _E, _E, _E},
          {_E, _E, _E, _E, _E},
      },

  };

  const PixelType FONT_DIGITS_3x5[11 + 3 + 1][NUM_PIXELS_HIGH][3] = {
      {
          {XX, XX, XX},
          {XX, _E, XX},
          {XX, _E, XX},
          {XX, _E, XX},
          {XX, XX, XX},
      },
      {
          {_E, _E, TL},
          {_E, _E, XX},
          {_E, _E, XX},
          {_E, _E, XX},
          {_E, _E, XX},
      },
      {
          {TL, XX, TR},
          {_E, _E, XX},
          {TL, XX, Br},
          {XX, _E, _E},
          {XX, XX, XX},
      },
      {
          {TL, XX, TR},
          {_E, _E, XX},
          {_E, XX, XX},
          {_E, _E, XX},
          {BL, XX, Br},
      },
      {
          {XX, _E, XX},
          {XX, _E, XX},
          {XX, XX, XX},
          {_E, _E, XX},
          {_E, _E, XX},
      },
      {
          {XX, XX, XX},
          {XX, _E, _E},
          {XX, XX, XX},
          {_E, _E, XX},
          {XX, XX, XX},
      },
      {
          {XX, XX, XX},
          {XX, _E, _E},
          {XX, XX, XX},
          {XX, _E, XX},
          {XX, XX, XX},
      },
      {
          {XX, XX, XX},
          {_E, _E, XX},
          {_E, _E, XX},
          {_E, _E, XX},
          {_E, _E, XX},
      },
      {
          {XX, XX, XX},
          {XX, _E, XX},
          {XX, XX, XX},
          {XX, _E, XX},
          {XX, XX, XX},
      },
      {
          {XX, XX, XX},
          {XX, _E, XX},
          {XX, XX, XX},
          {_E, _E, XX},
          {XX, XX, XX},
      },
      {
          // % = 10
          {XX, _E, XX},
          {_E, _E, XX},
          {_E, XX, _E},
          {XX, _E, _E},
          {XX, _E, XX},
      },
      {
          // O
          {XX, XX, XX},
          {XX, _E, XX},
          {XX, _E, XX},
          {XX, _E, XX},
          {XX, XX, XX},
      },
      {
          // F
          {XX, XX, XX},
          {XX, _E, _E},
          {XX, XX, _E},
          {XX, _E, _E},
          {XX, _E, _E},
      },
      {
          // N
          {XX, XX, _E},
          {XX, _E, XX},
          {XX, _E, XX},
          {XX, _E, XX},
          {XX, _E, XX},
      },
      {
          // MISSING CHAR
          {_E, _E, _E},
          {_E, _E, _E},
          {XX, XX, XX},
          {_E, _E, _E},
          {_E, _E, _E},
      },

  };

public:
  enum SpecialChar
  {
    LETTER_O = 12,
    LETTER_F,
    LETTER_N,
    LETTER_MISSING
  };

  enum ScreenLine
  {
    LINE1_OF_3,
    LINE2_OF_3,
    LINE3_OF_3,
    LINE1_OF_2,
    LINE2_OF_2,
  };

  SlantyDigit(TFT_eSPI *tft, uint8_t pixel_size, uint8_t spacing, uint32_t bgColour)
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
    uint8_t char_width = NUM_PIXELS_WIDE * _pixel_size;
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
  int get_str_width(const char *number)
  {
    uint8_t num_chars = strlen(number);
    uint8_t char_width = NUM_PIXELS_WIDE * _pixel_size;
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
  //--------------------------------------------------------------------------------
  int getWidth(const char *number)
  {
    return get_str_width(number);
  }
  //--------------------------------------------------------------------------------
  int getHeight()
  {
    return 5 * _pixel_size;
  }
  //--------------------------------------------------------------------------------

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
        draw_number(ch - '0', cursor_x, y, _pixel_size);
        cursor_x += NUM_PIXELS_WIDE * _pixel_size + spc;
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
        cursor_x += NUM_PIXELS_WIDE * _pixel_size + spc;
      }
      else if (ch == '%')
      {
        draw_number(9 + XX, cursor_x, y, _pixel_size);
        cursor_x += (NUM_PIXELS_WIDE * _pixel_size) + spc;
      }
    }
    // units
    if (unitsNotEmpty)
    {
      int unitsY = y + (_pixel_size * 5 - _tft->fontHeight());
      _tft->drawString(units, cursor_x + 10, unitsY);
    }
  }
  //--------------------------------------------------------------------------------

  void draw_float(uint8_t datum, char *number, char *units = "")
  {
    bool unitsNotEmpty = units[0] != '\0';
    int unitsWidth = unitsNotEmpty
                         ? (int)_tft->textWidth(units)
                         : 0;
    int width = get_str_width(number) + unitsWidth;

    draw_float(getX(datum, width), _getY(datum), number, units);
  }
  //--------------------------------------------------------------------------------

  void draw_float(uint8_t datum, ScreenLine line, char *number, char *units)
  {
    bool unitsNotEmpty = units[0] != '\0';
    int unitsWidth = unitsNotEmpty
                         ? (int)_tft->textWidth(units)
                         : 0;
    int width = get_str_width(number) + unitsWidth;

    draw_float(getX(datum, width), _getY(line), number, units);
  }
  //--------------------------------------------------------------------------------

  void drawText(int x, int y, const char *text)
  {
    int number_len = strlen(text);

    uint8_t cursor_x = x;

    for (int i = 0; i < number_len; i++)
    {
      char ch = text[i];
      uint8_t spc = i == number_len - 1 ? 0 : _spacing;
      uint8_t idx = 0;

      if (ch == 'O')
        idx = SpecialChar::LETTER_O - 1;
      else if (ch == 'F')
        idx = SpecialChar::LETTER_F - 1;
      else if (ch == 'N')
        idx = SpecialChar::LETTER_N - 1;
      else if (0 < int(ch) && int(ch) < 9)
        idx = int(ch);
      else
        idx = SpecialChar::LETTER_MISSING - 1;
      draw_number(idx, cursor_x, y, _pixel_size);
      cursor_x += NUM_PIXELS_WIDE * _pixel_size + spc;
    }
  }

  //--------------------------------------------------------------------------------
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
  //--------------------------------------------------------------------------------

private:
  TFT_eSPI *_tft;
  uint32_t _bgColour;
  uint8_t _pixel_size, _spacing;

  void draw_pixel(PixelType t, int x, int y, uint8_t pixelHeight)
  {
    const uint8_t X = 0, Y = 1;
    int tl[] = {x, y};
    int tr[] = {x + _pixel_size, y};
    int bl[] = {x, y + pixelHeight};
    int br[] = {x + _pixel_size, y + pixelHeight};

    switch (t)
    {
    case XX:
      _tft->fillRect(x, y, _pixel_size, pixelHeight, TFT_WHITE);
      break;
    case TL:
      _tft->fillTriangle(bl[X], bl[Y], br[X], br[Y], tr[X], tr[Y], TFT_WHITE);
      break;
    case TR:
      _tft->fillTriangle(tl[X], tl[Y], bl[X], bl[Y], br[X], br[Y], TFT_WHITE);
      break;
    case BL:
      _tft->fillTriangle(tl[X], tl[Y], tr[X], tr[Y], br[X], br[Y], TFT_WHITE);
      break;
    case Br:
      _tft->fillTriangle(tl[X], tl[Y], tr[X], tr[Y], bl[X], bl[Y], TFT_WHITE);
      break;
    default:
      _tft->fillRect(x, y, _pixel_size, pixelHeight, TFT_BLACK);
      break;
    }
  }

  void draw_number(
      uint8_t digit,
      uint8_t x,
      uint8_t y,
      uint8_t _pixel_size = 1)
  {
    // Serial.printf("-----\n");
    for (int xx = 0; xx < NUM_PIXELS_WIDE; xx++)
    {
      int y1 = y;
      for (int yy = 0; yy < NUM_PIXELS_HIGH; yy++)
      {
        int x1 = x + (xx * _pixel_size);
        bool smallLine = yy % 2 == 0;
        uint8_t smallP = _pixel_size * 0.6;
        uint8_t bigP = _pixel_size * 1.4;

        PixelType type = FONT_DIGITS_4x5[digit][yy][xx];
        draw_pixel(type, x1, y1, smallLine ? smallP : bigP);

        y1 += smallLine ? smallP : bigP;
      }
    }
  }

  int _getY(ScreenLine line)
  /*
  returns the top of the line
  */
  {
    const uint8_t lines3margin = 3, lines2margin = 10;
    uint8_t digitHeight = _pixel_size * NUM_PIXELS_HIGH;
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