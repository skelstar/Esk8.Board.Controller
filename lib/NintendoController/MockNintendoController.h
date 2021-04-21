#pragma once

#define BUTTONS_NUMBER 12
#define I2C_ADDR 0x52

#define NintendoController_h

class NintendoController
{
public:
  static const int BUTTON_UP = 0;
  static const int BUTTON_RIGHT = 1;
  static const int BUTTON_DOWN = 2;
  static const int BUTTON_LEFT = 3;
  static const int BUTTON_A = 4;
  static const int BUTTON_B = 5;
  // static const int BUTTON_X = 6;
  // static const int BUTTON_Y = 7;
  // static const int BUTTON_L = 8;
  // static const int BUTTON_R = 9;
  static const int BUTTON_START = 10;
  static const int BUTTON_SELECT = 11;
  static const int BUTTON_COUNT = 12;
  static const int BUTTON_NONE = 99;

  static const char *getButton(uint8_t button)
  {
    switch (button)
    {
    case BUTTON_UP:
      return "BUTTON_UP";
    case BUTTON_RIGHT:
      return "BUTTON_RIGHT";
    case BUTTON_DOWN:
      return "BUTTON_DOWN";
    case BUTTON_LEFT:
      return "BUTTON_LEFT";
    case BUTTON_A:
      return "BUTTON_A";
    case BUTTON_B:
      return "BUTTON_B";
    case BUTTON_START:
      return "BUTTON_START";
    case BUTTON_SELECT:
      return "BUTTON_SELECT";
    case BUTTON_COUNT:
      return "BUTTON_COUNT";
    };
    return "OUT OF RANGE NintendoController::getButton()";
  }

  static const int BUTTON_PRESSED = 1;
  static const int BUTTON_RELEASED = 0;

  typedef void (*ButtonEventCallback)(uint8_t button);
  typedef uint8_t (*MockGetButtonEventCallback)();

  bool init()
  {
    if (_mockGetButtonEventCallback == nullptr)
      DEBUG("ERROR: MockNintendoController _mockGetButtonEventCallback has not been initiailised!");

    return true;
  }

  /*
  returns true if new button pressed
  */
  bool update(xSemaphoreHandle mutex, TickType_t ticks)
  {
    uint8_t buttonPressed = _mockGetButtonEventCallback != nullptr
                                ? _mockGetButtonEventCallback()
                                : BUTTON_NONE;

    // check just pressed
    if (buttonStates[buttonPressed] != 1)
    {
      for (int i = 0; i < BUTTON_COUNT; i++)
        buttonStates[i] = buttonPressed == i;
      return true;
    }
    return false;
  }

  bool is_pressed(int button_index)
  {
    return buttonStates[button_index];
  }

  uint8_t *get_buttons()
  {
    return buttonStates;
  }

  void reset_buttons()
  {
    for (int i = 0; i < BUTTON_COUNT; i++)
      buttonStates[i] = 0;
  }

  void setMockGetButtonEventCallback(MockGetButtonEventCallback cb)
  {
    _mockGetButtonEventCallback = cb;
  }

private:
  MockGetButtonEventCallback _mockGetButtonEventCallback = nullptr;
  int address = I2C_ADDR;
  uint8_t buttonStates[BUTTON_COUNT];
};