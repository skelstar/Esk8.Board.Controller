

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

    classicButtonsQueue = new Queue::Manager(xClassicButtons, 5);
  }

  void loop()
  {
    classic.update(); // Get new data from the controller

    classic.debug();

    if (classic.was_released(classic.BUTTON_A))
      Serial.printf("BUTTON A released!\n");
  }
}