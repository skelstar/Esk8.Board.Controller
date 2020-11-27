
#ifndef TFT_H
#include <tft.h>
#endif

void displayStateEventCb(int ev)
{
#ifdef PRINT_DISP_STATE_EVENT
  Serial.printf("--> disp: %s\n", DispState::names[ev]);
#endif
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

      DispState::Event displayevent = (DispState::Event)displayChangeQueueManager->read();
      switch (displayevent)
      {
      case DispState::NO_EVENT:
      case NO_QUEUE_EVENT:
        break;
      case DispState::UPDATE:
        update_display = true;
        break;
      default:
        lastDispEvent = displayevent;
        displayState->trigger(displayevent);
        break;
      }

      uint8_t buttonEvent = buttonQueue->read();
      switch (buttonEvent)
      {
      case SINGLE:
        lastDispEvent = DispState::PRIMARY_SINGLE_CLICK;
        displayState->trigger(DispState::PRIMARY_SINGLE_CLICK);
        break;
      case DOUBLE:
        lastDispEvent = DispState::PRIMARY_DOUBLE_CLICK;
        displayState->trigger(DispState::PRIMARY_DOUBLE_CLICK);
        break;
      case TRIPLE:
        lastDispEvent = DispState::PRIMARY_TRIPLE_CLICK;
        displayState->trigger(DispState::PRIMARY_TRIPLE_CLICK);
        break;
      case 99:
        break;
      }
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//-----------------------------------------------------

void createDisplayTask0(uint8_t core, uint8_t priority)
{
  xTaskCreatePinnedToCore(
      display_task_0,
      "display_task_0",
      10000,
      NULL,
      priority,
      NULL,
      core);
}