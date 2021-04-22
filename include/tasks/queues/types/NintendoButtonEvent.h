#pragma once

#include <tasks/queues/types/QueueBase.h>

class NintendoButtonEvent : public QueueBase
{
public:
  uint8_t button;
  uint8_t state;
  bool changed;

public:
  NintendoButtonEvent() : QueueBase()
  {
    name = "NintendoButtonEvent";
  }

  NintendoButtonEvent(uint8_t button, uint8_t state) : QueueBase()
  {
    button = button;
    state = state;
    name = "NintendoButtonEvent";
  }

  void printSend(const char *queueName = nullptr)
  {
    // TODO put button into here
    Serial.printf("[Queue|%s| SEND |%lums] event_id: %lu\n",
                  queueName != nullptr ? queueName : name,
                  millis(),
                  event_id);
  }

  // static const char *getButton(uint8_t button)
  // {
  //   switch (button)
  //   {
  //   case NintendoController::BUTTON_UP:
  //     return "BUTTON_UP";
  //   case NintendoController::BUTTON_RIGHT:
  //     return "BUTTON_RIGHT";
  //   case NintendoController::BUTTON_DOWN:
  //     return "BUTTON_DOWN";
  //   case NintendoController::BUTTON_LEFT:
  //     return "BUTTON_LEFT";
  //   case NintendoController::BUTTON_A:
  //     return "BUTTON_A";
  //   case NintendoController::BUTTON_B:
  //     return "BUTTON_B";
  //   case NintendoController::BUTTON_START:
  //     return "BUTTON_START";
  //   case NintendoController::BUTTON_SELECT:
  //     return "BUTTON_SELECT";
  //   case NintendoController::BUTTON_COUNT:
  //     return "BUTTON_COUNT";
  //   };
  //   return "OUT OF RANGE NintendoController::getButton()";
  // }
};
