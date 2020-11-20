
#ifndef TFT_H
#include <tft.h>
#endif

void displayStateEventCb(int ev)
{
}

void display_task_0(void *pvParameters)
{
  setupLCD();

  displayState = new Fsm(&dispState_searching);
  displayState->setEventTriggeredCb(displayStateEventCb);

  displayState_addTransitions();

  Serial.printf("display_task_0 running on core %d\n", xPortGetCoreID());

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

      DispStateEvent displayevent = (DispStateEvent)displayChangeQueueManager->read();
      switch (displayevent)
      {
      case DISP_EV_NO_EVENT:
      case 99:
        break;
      case DISP_EV_UPDATE:
        update_display = true;
        break;
      default:
        lastDispEvent = displayevent;
        displayState->trigger(displayevent);
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
