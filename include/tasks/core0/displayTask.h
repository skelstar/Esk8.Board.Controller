
#ifndef TFT_H
#include <tft.h>
#endif

void displayStateEventCb(int ev)
{
#ifdef PRINT_DISP_STATE_EVENT
  Serial.printf("--> disp: %s\n", DispState::names[ev]);
#endif
}

void displayTask(void *pvParameters)
{
  setupLCD();

  displayState = new Fsm(&dispState_searching);
  displayState->setEventTriggeredCb(displayStateEventCb);

  displayState_addTransitions();

  Serial.printf("displayTask running on core %d\n", xPortGetCoreID());

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

      DispState::Event displayevent = displayQueue->read<DispState::Event>();
      switch (displayevent)
      {
      case DispState::NO_EVENT:
        break;
      case DispState::UPDATE:
        update_display = true;
        break;
      default:
        lastDispEvent = displayevent;
        displayState->trigger(displayevent);
        break;
      }

      ButtonClickType buttonEvent = buttonQueue->read<ButtonClickType>();
      switch (buttonEvent)
      {
      case NO_CLICK:
        break;
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
      displayTask,
      "displayTask",
      10000,
      NULL,
      priority,
      NULL,
      core);
}