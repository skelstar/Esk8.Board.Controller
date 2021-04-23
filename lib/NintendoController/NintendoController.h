#pragma once

#ifndef NintendoController_h
#define NintendoController_h

#define BUTTONS_NUMBER 12
#define I2C_ADDR 0x52

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

  static const int BUTTON_PRESSED = 1;
  static const int BUTTON_RELEASED = 0;

  typedef void (*ButtonEventCallback)(uint8_t button);

  bool init(bool printWarnings = true);
  bool update(xSemaphoreHandle mutex, TickType_t ticks);
  bool is_pressed(int button_index);
  bool was_pressed(int button_index);
  bool was_released(int button_index);
  uint8_t *get_buttons();
  void debug();
  int address = I2C_ADDR;
  uint8_t buttonStates[BUTTON_COUNT];
  ButtonEventCallback _buttonPressed_cb = nullptr;
  ButtonEventCallback _buttonReleased_cb = nullptr;

  static const int BUTTON_BYTES_UP = 0x0001;
  static const int BUTTON_BYTES_RIGHT = 0x8000;
  static const int BUTTON_BYTES_DOWN = 0x4000;
  static const int BUTTON_BYTES_LEFT = 0x0002;
  static const int BUTTON_BYTES_A = 0x0010;
  static const int BUTTON_BYTES_B = 0x0040;
  static const int BUTTON_BYTES_X = 0x0008;
  static const int BUTTON_BYTES_Y = 0x0020;
  static const int BUTTON_BYTES_L = 0x2000;
  static const int BUTTON_BYTES_R = 0x0200;
  static const int BUTTON_BYTES_START = 0x0400;
  static const int BUTTON_BYTES_SELECT = 0x1000;

  struct button
  {
    int bytes;
    bool pressed;
    bool wasPressed;
    bool wasReleased;
  };

  button old_buttons[BUTTON_COUNT];

  button buttons[BUTTONS_NUMBER] = {
      {BUTTON_BYTES_UP, false},
      {BUTTON_BYTES_RIGHT, false},
      {BUTTON_BYTES_DOWN, false},
      {BUTTON_BYTES_LEFT, false},
      {BUTTON_BYTES_A, false},
      {BUTTON_BYTES_B, false},
      {BUTTON_BYTES_X, false},
      {BUTTON_BYTES_Y, false},
      {BUTTON_BYTES_L, false},
      {BUTTON_BYTES_R, false},
      {BUTTON_BYTES_START, false},
      {BUTTON_BYTES_SELECT, false}};

  void reset_buttons();

  static const char *getButtonName(uint8_t button)
  {
    switch (button)
    {
    case NintendoController::BUTTON_UP:
      return "BUTTON_UP";
    case NintendoController::BUTTON_RIGHT:
      return "BUTTON_RIGHT";
    case NintendoController::BUTTON_DOWN:
      return "BUTTON_DOWN";
    case NintendoController::BUTTON_LEFT:
      return "BUTTON_LEFT";
    case NintendoController::BUTTON_A:
      return "BUTTON_A";
    case NintendoController::BUTTON_B:
      return "BUTTON_B";
    case NintendoController::BUTTON_START:
      return "BUTTON_START";
    case NintendoController::BUTTON_SELECT:
      return "BUTTON_SELECT";
    case NintendoController::BUTTON_COUNT:
      return "BUTTON_COUNT";
    case NintendoController::BUTTON_NONE:
      return "BUTTON_NONE";
    }
    return "OUT OF RANGE: getButtonName()";
  }
};

#endif