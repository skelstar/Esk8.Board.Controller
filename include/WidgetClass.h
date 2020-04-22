#ifndef Arduino_h
#include <Arduino.h>
#endif
#ifndef TFT_H
#include <tft.h>
#endif

enum WidgetSize
{
  Wide,
  Normal
};

enum WidgetPos
{
  TOP_LEFT,
  TOP_CENTRE,
  TOP_RIGHT,
  BOTTOM_LEFT,
  BOTTOM_CENTRE,
  BOTTOM_RIGHT
};

template <typename T>
class WidgetClass
{
public:
  WidgetClass()
  {
    tft.setTextColor(TFT_WHITE);
  }

  WidgetClass(uint32_t bgColour, uint32_t fgColour = TFT_WHITE)
  {
    _bgColour = bgColour;
    tft.setTextColor(fgColour);
  }

  WidgetClass(uint8_t x,
              uint8_t y,
              WidgetSize size,
              uint32_t bgColour,
              uint32_t fgColour = TFT_WHITE)
  {
    _x = x;
    _y = y;
    _width = size == Wide ? (LCD_WIDTH / 3) * 2 : LCD_WIDTH / 2;
    _height = LCD_HEIGHT / 2;
    tft.setTextColor(fgColour);
  }

  void setStatusLevels(T warn, T crit, bool statusDirectionSwapped = false)
  {
    _warn = warn;
    _crit = crit;
    _statusDirSwapped = statusDirectionSwapped;
    _levelsSet = true;
  }

  void setPosition(WidgetPos pos, WidgetSize size)
  {
    switch (pos)
    {
    case TOP_LEFT:
    case BOTTOM_LEFT:
      _x = 0;
      break;
    case TOP_CENTRE:
    case BOTTOM_CENTRE:
      _x = LCD_WIDTH / 3;
      break;
    case TOP_RIGHT:
    case BOTTOM_RIGHT:
      _x = (LCD_WIDTH / 3) * 2;
      break;
    }
    _y = pos == TOP_LEFT || pos == TOP_CENTRE || pos == TOP_RIGHT
             ? 0
             : LCD_HEIGHT / 2;

    _width = size == Wide ? (LCD_WIDTH / 3) * 2 : LCD_WIDTH / 3;
    _height = LCD_HEIGHT / 2;
  }

  void setForegroundColour(uint32_t colour)
  {
    tft.setTextColor(colour);
  }

  void setBackgroundColour(uint32_t colour)
  {
    _bgColour = colour;
  }

  void draw(TFT_eSPI *tft, T number, const char *title, bool border = false)
  {
    if (_levelsSet)
    {
      tft->fillRect(_x, _y, _width, _height, status_colours[_getStatusLevel(number)]);
    }
    else if (_bgColour != 0)
    {
      tft->fillRect(_x, _y, _width, _height, _bgColour);
    }

    if (border)
    {
      tft->drawRect(_x, _y, _width, _height, TFT_WHITE);
    }

    tft->setTextDatum(TC_DATUM);
    tft->setTextSize(3);
    if (std::is_same<T, float>::value)
    {
      tft->drawFloat(number, 1, _x + (_width / 2), _y + 10);
    }
    else
    {
      tft->drawNumber(number, _x + (_width / 2), _y + 10);
    }

    tft->setTextDatum(BC_DATUM);
    tft->setTextSize(2);
    tft->drawString(title, _x + (_width / 2), _y + _height - 6);
  }

private:
  MessageStatus _getStatusLevel(T val)
  {
    bool swapped = _statusDirSwapped;
    return ((val >= _crit && !swapped) || (val <= _crit && swapped))
               ? CRITICAL
               : ((val >= _warn && !swapped) || (val <= _warn && swapped))
                     ? WARNING
                     : OKAY;
  }

  uint8_t _pixelWidth, _spacing;
  uint8_t _x, _y, _width, _height;
  uint32_t _bgColour;
  bool _levelsSet = false, _statusDirSwapped = false;
  T _warn = 0, _crit = 0;
};