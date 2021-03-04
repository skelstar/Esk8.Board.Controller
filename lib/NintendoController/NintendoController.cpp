#include <Wire.h>
#include "Arduino.h"
#include "NintendoController.h"

// NintendoController::NintendoController()
// {
//   Wire.begin();
// }

void NintendoController::init()
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
  Wire.endTransmission();
  delay(10);

  reset_buttons();

  // notice that doesn't trigger any error
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
  }
}

bool NintendoController::is_pressed()
{
  for (int i = 0; i < BUTTONS_NUMBER; i++)
  {
    if (this->buttons[i].pressed)
      return true;
  }
  return false;
}

bool NintendoController::is_pressed(int button_index)
{
  return (button_index >= 0 && button_index <= BUTTONS_NUMBER && this->buttons[button_index].pressed);
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

void NintendoController::debug()
{
  String debug_str = "";

  if (this->buttons[0].pressed)
    debug_str += "UP ";
  if (this->buttons[1].pressed)
    debug_str += "RIGHT ";
  if (this->buttons[2].pressed)
    debug_str += "DOWN ";
  if (this->buttons[3].pressed)
    debug_str += "LEFT ";
  if (this->buttons[4].pressed)
    debug_str += "A ";
  if (this->buttons[5].pressed)
    debug_str += "B ";
  if (this->buttons[6].pressed)
    debug_str += "X ";
  if (this->buttons[7].pressed)
    debug_str += "Y ";
  if (this->buttons[8].pressed)
    debug_str += "L ";
  if (this->buttons[9].pressed)
    debug_str += "R ";
  if (this->buttons[10].pressed)
    debug_str += "START ";
  if (this->buttons[11].pressed)
    debug_str += "SELECT ";

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