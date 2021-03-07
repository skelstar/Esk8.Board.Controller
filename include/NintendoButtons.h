

#include <Wire.h>

#include <NintendoController.h>
#include <QueueManager.h>

xQueueHandle xClassicButtons;
Queue::Manager *classicButtonsQueue;

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

  void init()
  {
    classic.init();
    classic.setButtonPressedCb([](uint8_t button) {
      Serial.printf("button %d was pressed\n", button);
    });
    classic.setButtonReleasedCb([](uint8_t button) {
      Serial.printf("button %d was released\n", button);
    });

    classicButtonsQueue = new Queue::Manager(xClassicButtons, 5);
  }

  void loop()
  {
    classic.update(); // Get new data from the controller
  }
}