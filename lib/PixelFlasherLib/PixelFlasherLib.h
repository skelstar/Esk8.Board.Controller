
#ifndef ADAFRUIT_NEOPIXEL_H
#include <Adafruit_NeoPixel.h>
#endif

#ifndef FLASHER_MID_INTERVAL_MS
#define FLASHER_MID_INTERVAL_MS 2000
#endif
#ifndef FLASHER_OFF_INTERVAL_MS
#define FLASHER_OFF_INTERVAL_MS 50
#endif
#ifndef FLASHER_BETWEEN_FLASHES_MS
#define FLASHER_BETWEEN_FLASHES_MS 250
#endif

class PixelFlasherLib
{
public:
  PixelFlasherLib(Adafruit_NeoPixel *pixel, uint8_t pixelpin)
  {
    _pixel = pixel;
    _pixelPin = pixelpin;

    _time_to_change = FLASHER_MID_INTERVAL_MS;
    _currentState = FlashCommand::ON;
  }

  /* cycles = 0 is infinite */
  void setFlashes(uint8_t num, uint8_t cycles = 0)
  {
    _numFlashes = num;
    _cycles = cycles;
    _infinite = cycles == 0;
    _flashCount = 0;
  }

  void blink()
  {
    setFlashes(1, 1);
  }

  void blinks(uint8_t num = 1)
  {
    setFlashes(num, 1);
  }

  void setColour(uint32_t colour)
  {
    _currentColour = colour;
    if (_currentState == ON)
    {
      _pixel->setPixelColor(_pixelPin, _currentColour);
      _pixel->show();
    }
  }

  void loop()
  {
    if (_since_last_state_change > _time_to_change)
    {
      _since_last_state_change = 0;
      _flashEvent();
    }
  }

private:
  enum FlashCommand
  {
    ON,
    OFF
  };

  uint8_t _pixelPin = 0,
          _numFlashes = 0,
          _cycles = 0,
          _flashCount = 0;
  bool _infinite;
  volatile FlashCommand _currentState;
  uint32_t _currentColour;
  volatile uint16_t _time_to_change;
  elapsedMillis _since_last_state_change;

  // Tasker *_flasher;
  Adafruit_NeoPixel *_pixel;

  uint32_t COLOUR_OFF = _pixel->Color(0, 0, 0);

  void _setNextState(FlashCommand state)
  {
    _currentState = state;
  }

  void _flashEvent()
  {
    switch (_currentState)
    {
    case FlashCommand::ON:
      if (_numFlashes == 0 || (_cycles == 0 && !_infinite))
      {
        return;
      }
      if (_flashCount == _numFlashes)
      {
        if (_cycles > 0 || _infinite)
        {
          _flashCount = 0;
          _cycles = !_infinite ? _cycles - 1 : 0;
        }
        if (_cycles == 0 && !_infinite)
        {
          return;
        }
      }
      _pixel->setPixelColor(0, _pixel->Color(0, 0, 0));
      _pixel->show();
      _setNextState(FlashCommand::OFF);
      _time_to_change = FLASHER_OFF_INTERVAL_MS;
      break;
    case FlashCommand::OFF:
      if (_flashCount == _numFlashes)
      {
        return;
      }
      _flashCount++;

      _pixel->setPixelColor(0, _currentColour);
      _pixel->show();
      _time_to_change = _flashCount < _numFlashes
                            ? FLASHER_BETWEEN_FLASHES_MS
                            : FLASHER_MID_INTERVAL_MS;
      _setNextState(FlashCommand::ON);
      break;
    }
  }
};