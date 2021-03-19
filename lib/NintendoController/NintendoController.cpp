#include <Wire.h>
#include "Arduino.h"
#include "NintendoController.h"

// NintendoController::NintendoController()
// {
//   Wire.begin();
// }

bool NintendoController::init()
{
  // Not required for NES mini controller
  // See http://wiibrew.org/wiki/Wiimote/Extension_Controllers

  // send 0x55 to 0xF0
  Wire.beginTransmission(this->address);
  Wire.write(0xF0);
  Wire.write(0x55);
  Wire.endTransmission();
  delay(10);

  // send 0x00 to 0xFB
  Wire.beginTransmission(this->address);
  Wire.write(0xFB);
  Wire.write(0x00);
  bool success = Wire.endTransmission() == 0;
  delay(10);

  reset_buttons();

  return success;
}

void NintendoController::update()
{
  Wire.beginTransmission(this->address);
  if (Wire.endTransmission() != 0)
  {
    // try to reconnect
    this->init();
  }
  else
    delay(10);

  // send 0x00 to ask for buttons, then read the results
  Wire.beginTransmission(this->address);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(10);

  int current_byte = 0;
  int button_bytes = 0;
  Wire.requestFrom(this->address, 6);
  while (Wire.available())
  {
    int byte_read = Wire.read();
    if (current_byte == 4)
    {
      button_bytes |= (255 - byte_read) << 8;
    }
    else if (current_byte == 5)
    {
      button_bytes |= (255 - byte_read);
    }
    current_byte++;
  }

  for (int i = 0; i < BUTTONS_NUMBER; i++)
  {
    this->old_buttons[i].pressed = this->buttons[i].pressed;
    this->buttons[i].pressed = ((button_bytes & this->buttons[i].bytes) == this->buttons[i].bytes);
    if (this->was_pressed(i) && _buttonPressed_cb != nullptr)
      _buttonPressed_cb(i);
    if (this->was_released(i) && _buttonReleased_cb != nullptr)
      _buttonReleased_cb(i);
  }
}

bool NintendoController::is_pressed(int button_index)
{
  return (button_index >= 0 &&
          button_index <= BUTTONS_NUMBER &&
          this->buttons[button_index].pressed);
}

bool NintendoController::was_pressed(int button_index)
{
  return (
      button_index >= 0 &&
      button_index <= BUTTONS_NUMBER &&
      this->buttons[button_index].pressed == 1 &&
      this->old_buttons[button_index].pressed == 0);
}

bool NintendoController::was_released(int button_index)
{
  return (
      button_index >= 0 &&
      button_index <= BUTTONS_NUMBER &&
      this->buttons[button_index].pressed == 0 &&
      this->old_buttons[button_index].pressed == 1);
}

uint8_t *NintendoController::get_buttons()
{
  static uint8_t btns[BUTTON_COUNT];
  for (int i = 0; i < BUTTON_COUNT; i++)
    btns[i] = this->buttons[i].pressed;
  return btns;
}

void NintendoController::setButtonPressedCb(ButtonEventCallback cb)
{
  _buttonPressed_cb = cb;
}

void NintendoController::setButtonReleasedCb(ButtonEventCallback cb)
{
  _buttonReleased_cb = cb;
}

void NintendoController::debug()
{
  String debug_str = "";

  if (was_pressed(BUTTON_UP))
    debug_str += "UP pressed";
  if (was_pressed(BUTTON_RIGHT))
    debug_str += "RIGHT pressed";
  if (was_pressed(BUTTON_DOWN))
    debug_str += "DOWN pressed";
  if (was_pressed(BUTTON_LEFT))
    debug_str += "LEFT pressed";
  if (was_pressed(BUTTON_A))
    debug_str += "A pressed";
  if (was_pressed(BUTTON_B))
    debug_str += "B pressed";
  if (was_pressed(BUTTON_START))
    debug_str += "START pressed";
  if (was_pressed(BUTTON_SELECT))
    debug_str += "SELECT pressed";
  if (was_released(BUTTON_UP))
    debug_str += "UP released";
  if (was_released(BUTTON_RIGHT))
    debug_str += "RIGHT released";
  if (was_released(BUTTON_DOWN))
    debug_str += "DOWN released";
  if (was_released(BUTTON_LEFT))
    debug_str += "LEFT released";
  if (was_released(BUTTON_A))
    debug_str += "A released";
  if (was_released(BUTTON_B))
    debug_str += "B released";
  if (was_released(BUTTON_START))
    debug_str += "START released";
  if (was_released(BUTTON_SELECT))
    debug_str += "SELECT released";

  if (debug_str != "")
    Serial.println(debug_str);
}

void NintendoController::reset_buttons()
{
  for (int i = 0; i < BUTTONS_NUMBER; i++)
  {
    this->buttons[i].pressed = false;
    this->buttons[i].wasPressed = false;
    this->buttons[i].wasReleased = false;
    this->old_buttons[i].pressed = false;
    this->old_buttons[i].wasPressed = false;
    this->old_buttons[i].wasReleased = false;
  }
}