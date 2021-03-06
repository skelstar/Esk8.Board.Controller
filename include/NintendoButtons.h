

#include <Wire.h>

#include <NintendoController.h>
#include <QueueManager.h>

xQueueHandle xClassicButtons;
Queue::Manager *classicButtonsQueue;

NintendoController classic;

namespace ClassicButtons
{
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