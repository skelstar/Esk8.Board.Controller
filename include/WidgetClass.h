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
  WidgetClass(WidgetPos pos, WidgetSize size, uint32_t bgColour = TFT_BLUE, uint32_t fgColour = TFT_WHITE)
  {
    _bgColour = bgColour;
    _spr.setTextColor(fgColour);
    _spr.setTextFont(1);
    _setPosition(pos, size);
    _spr.fillScreen(bgColour);
  }

  void reset()
  {
    _first = true;
  }

  void setStatusLevels(T warn, T crit, bool statusDirectionSwapped = false)
  {
    _warn = warn;
    _crit = crit;
    _statusDirSwapped = statusDirectionSwapped;
    _levelsSet = true;
  }

  void setOnlyShowNonZero(bool show)
  {
    _showOnlyNonZero = show;
  }

  //--------------------------------------------------------------------------------

  void setForegroundColour(uint32_t colour)
  {
    _spr.setTextColor(colour);
  }
  //--------------------------------------------------------------------------------

  void setBackgroundColour(uint32_t colour)
  {
    _bgColour = colour;
  }
  //--------------------------------------------------------------------------------

  void draw(T number, const char *title, bool border = false)
  {
    if (number != _lastValue || _first)
    {
      if (_showOnlyNonZero && number == 0)
      {
        // clear widget
        _spr.fillScreen(TFT_BLUE);
        _spr.pushSprite(_x, _y);
      }
      else
      {
        _first = false; // force displaying first time
        _spr.fillScreen(_bgColour);
        _spr.setTextFont(1);

        _spr.setTextDatum(TC_DATUM);
        _spr.setTextSize(3);
        if (std::is_same<T, float>::value)
        {
          _spr.drawFloat(number, 1, _width / 2, 12);
        }
        else
        {
          _spr.drawNumber(number, _width / 2, 12);
        }

        _spr.setTextDatum(BC_DATUM);
        _spr.setTextSize(2);
        _spr.drawString(title, _width / 2, _height - 6);
        _spr.pushSprite(_x, _y);
      }
    }
    else
    {
      // DEBUGVAL(title, number, _lastValue, _first);
    }
    _lastValue = number;
  }

#define BATT_WIDTH 36
#define BATT_HEIGHT 20
#define BORDER_SIZE 2
#define KNOB_HEIGHT 10
#define KNOB_WIDTH 4

  void drawBattery(BatteryLib batt)
  {
    uint32_t colour = TFT_WHITE;
    uint8_t x = _spr.width() / 2 - BATT_WIDTH / 2 - KNOB_WIDTH / 2;
    uint8_t y = _spr.height() / 2 - BATT_HEIGHT / 2;

    // knob
    _spr.fillRect(x, y + ((BATT_HEIGHT - KNOB_HEIGHT) / 2), KNOB_WIDTH, KNOB_HEIGHT, colour);

    // body - outline
    x += KNOB_WIDTH;
    uint8_t w = BATT_WIDTH;
    uint8_t h = BATT_HEIGHT;
    _spr.fillRect(x, y, w, h, colour);

    // body - empty inside
    x += BORDER_SIZE;
    y += BORDER_SIZE;
    w -= BORDER_SIZE * 2;
    h -= BORDER_SIZE * 2;
    _spr.fillRect(x, y, w, h, _bgColour);

    // capacity
    w -= BORDER_SIZE * 2;
    h -= BORDER_SIZE * 2;

    if (!batt.isCharging)
    {
      x += BORDER_SIZE;
      y += BORDER_SIZE;
      _spr.fillRect(x, y, w, h, colour); // solid
      // black rect for used part
      w = w * (1 - (batt.chargePercent / 100.0));
      _spr.fillRect(x, y, w, h, _bgColour);
      DEBUGVAL(batt.chargePercent);
    }
    else
    {
      x += BORDER_SIZE;
      y += BORDER_SIZE;
      // fill the batt
      _spr.fillRect(x, y, w, h, colour);
      // plus
      const int plus = 4, edge = 3;
      _spr.fillRect(x + (w / 2) - (edge + (plus / 2)),
                    y + (h / 2) - (plus / 2),
                    edge + plus + edge,
                    plus,
                    _bgColour);
      _spr.fillRect(x + (w / 2) - (plus / 2),
                    y + (h / 2) - (edge + (plus / 2)),
                    plus,
                    edge + plus + edge,
                    _bgColour);
    }
    _spr.pushSprite(_x, _y);
  }
  //--------------------------------------------------------------------------------

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

  void _setPosition(WidgetPos pos, WidgetSize size)
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
    _spr.createSprite(_width, _height);
  }

  uint8_t _pixelWidth, _spacing;
  uint8_t _x, _y, _width, _height;
  uint32_t _bgColour;
  bool _levelsSet = false, _statusDirSwapped = false, _first = true, _showOnlyNonZero = false;
  T _warn = 0, _crit = 0, _lastValue;
  TFT_eSprite _spr = TFT_eSprite(&tft);
};