
#ifndef TFT_H
#include <tft.h>
#endif

void display_task_0(void *pvParameters)
{
  RTOSUtils::printTaskDetails();

  setupLCD();

  displayState = new Fsm(&dispState_searching);
  displayState->setGetEventName(Disp::getName);

  displayState_addTransitions();

  displayState->run_machine();

  display_task_initialised = true;

#define READ_DISP_EVENT_QUEUE_PERIOD 100

  elapsedMillis sinceReadDispEventQueue;

  while (true)
  {
    if (sinceReadDispEventQueue > READ_DISP_EVENT_QUEUE_PERIOD)
    {
      sinceReadDispEventQueue = 0;
      displayState->run_machine();

      uint8_t ev = displayChangeQueueManager->read();
      if (ev >= Disp::Length && ev != NO_QUEUE_EVENT)
      {
        Serial.printf("WARNING: received a display event that is out of range\n");
      }
      switch (ev)
      {
      case Disp::NO_EVENT:
        break;
      case Disp::UPDATE:
        update_display = true;
        break;
      default:
        lastDispEvent = (Disp::Event)ev;
        displayState->trigger(ev);
        break;
      }

      uint8_t buttonEvent = buttonQueueManager->read();
      switch (buttonEvent)
      {
      case SINGLE:
        lastDispEvent = Disp::PRIMARY_SINGLE_CLICK;
        displayState->trigger(Disp::PRIMARY_SINGLE_CLICK);
        break;
      case DOUBLE:
        lastDispEvent = Disp::PRIMARY_DOUBLE_CLICK;
        displayState->trigger(Disp::PRIMARY_DOUBLE_CLICK);
        break;
      case TRIPLE:
        lastDispEvent = Disp::PRIMARY_TRIPLE_CLICK;
        displayState->trigger(Disp::PRIMARY_TRIPLE_CLICK);
        break;
      case 99:
        break;
      }
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
