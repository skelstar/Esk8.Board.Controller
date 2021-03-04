

#include <Wire.h>

#include <NintendoController.h>
#include <QueueManager.h>

xQueueHandle xClassicButtons;
Queue::Manager *classicButtonsQueue;

NintendoController classic;

namespace ClassicButtons
{
  enum QueueEvent
  {
    TR_BUTTON_UP,
    TR_BUTTON_RIGHT,
    TR_BUTTON_DOWN,
    TR_BUTTON_LEFT,
    TR_BUTTON_A,
    TR_BUTTON_B,
    TR_BUTTON_START,
    TR_BUTTON_SELECT
  };

  const char *getQueueEvent(uint8_t ev)
  {
    switch (ev)
    {
    case TR_BUTTON_UP:
      return "TR_BUTTON_UP";
    case TR_BUTTON_RIGHT:
      return "TR_BUTTON_RIGHT";
    case TR_BUTTON_DOWN:
      return "TR_BUTTON_DOWN";
    case TR_BUTTON_LEFT:
      return "TR_BUTTON_LEFT";
    case TR_BUTTON_A:
      return "TR_BUTTON_A";
    case TR_BUTTON_B:
      return "TR_BUTTON_B";
    case TR_BUTTON_START:
      return "TR_BUTTON_START";
    case TR_BUTTON_SELECT:
      return "TR_BUTTON_SELECT";
    }
    return "OUT OF RANGE: getQueueEvent()";
  }

  void init()
  {
    classic.init();

    classicButtonsQueue = new Queue::Manager(xClassicButtons, 5);

    classic.setPressedEventCb([](uint8_t button) {
      Serial.printf("button %s pressed\n", classic.getButton(button));
    });
    classic.setReleasedEventCb([](uint8_t button) {
      Serial.printf("button %s released\n", classic.getButton(button));
    });
  }

  void loop()
  {
    classic.update(); // Get new data from the controller
  }
}