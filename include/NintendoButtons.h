#pragma once

#include <Wire.h>
#include <NintendoController.h>
#include <QueueManager.h>

NintendoController classic;

namespace ClassicButtons
{
  const char *getButtonName(uint8_t button)
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
    }
    return "OUT OF RANGE: getButtonName()";
  }

  NintendoController::ButtonEventCallback _pressed_cb = nullptr, _released_cb = nullptr;

  void init()
  {
    if (mutex_I2C.take(__func__, TICKS_50))
    {
      if (!classic.init())
      {
        Serial.printf("ERROR: could not find Nintendo Buttons\n");
      }
      mutex_I2C.give(__func__);
    }
  }

  void setButtonReleasedCallback(NintendoController::ButtonEventCallback cb)
  {
    classic.setButtonReleasedCb(cb);
  }

  void setButtonPressedCallback(NintendoController::ButtonEventCallback cb)
  {
    classic.setButtonPressedCb(cb);
  }

  void loop()
  {
    if (mutex_I2C.take(__func__, TICKS_50))
    {
      classic.update(); // Get new data from the controller}
      mutex_I2C.give(__func__);
    }
  }

  bool buttonPressed(uint8_t button)
  {
    return classic.is_pressed(button);
  }

  bool throttle_enabled()
  {
    return classic.is_pressed(NintendoController::BUTTON_B);
  }
}