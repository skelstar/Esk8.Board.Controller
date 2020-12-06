
#ifndef TFT_H
#include <tft.h>
#endif

void display_task_0(void *pvParameters)
{
  setupLCD();

  displayState = new Fsm(&dispState_searching);

  displayState_addTransitions();

  RTOSUtils::printTaskDetails();

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
      if (ev >= DispStateEvent::DISP_EV_Length && ev != NO_QUEUE_EVENT)
      {
        Serial.printf("WARNING: received a display event that is out of range\n");
      }
      switch (ev)
      {
      case DISP_EV_NO_EVENT:
      case 99:
        break;
      case DISP_EV_UPDATE:
        update_display = true;
        break;
      default:
        lastDispEvent = (DispStateEvent)ev;
        displayState->trigger(ev);
        break;
      }

      uint8_t buttonEvent = buttonQueueManager->read();
      switch (buttonEvent)
      {
      case SINGLE:
        lastDispEvent = DISP_EV_PRIMARY_SINGLE_CLICK;
        displayState->trigger(DISP_EV_PRIMARY_SINGLE_CLICK);
        break;
      case DOUBLE:
        lastDispEvent = DISP_EV_PRIMARY_DOUBLE_CLICK;
        displayState->trigger(DISP_EV_PRIMARY_DOUBLE_CLICK);
        break;
      case TRIPLE:
        lastDispEvent = DISP_EV_PRIMARY_TRIPLE_CLICK;
        displayState->trigger(DISP_EV_PRIMARY_TRIPLE_CLICK);
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
