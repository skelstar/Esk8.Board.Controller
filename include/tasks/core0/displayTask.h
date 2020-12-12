
#ifndef TFT_H
#include <tft.h>
#endif

void displayStateEventCb(int ev)
{
  if (PRINT_DISP_STATE_EVENT)
    Serial.printf("--> disp: %s\n", DispState::getEvent(ev));
}

void displayTask(void *pvParameters)
{
  setupLCD();

  DisplayState::dispFsm.begin(&DisplayState::fsm, STATE_STRING_FORMAT);
  DisplayState::dispFsm.setGetStateNameCallback([](uint8_t id) {
    return DisplayState::getStateName(id);
  });
  DisplayState::addTransitions();

  Serial.printf("displayTask running on core %d\n", xPortGetCoreID());

  DisplayState::fsm.run_machine();

  display_task_initialised = true;

#define READ_DISP_EVENT_QUEUE_PERIOD 100

  elapsedMillis sinceReadDispEventQueue;

  while (true)
  {
    if (sinceReadDispEventQueue > READ_DISP_EVENT_QUEUE_PERIOD)
    {
      sinceReadDispEventQueue = 0;
      DisplayState::fsm.run_machine();

      DispState::Event displayevent = displayQueue->read<DispState::Event>();
      switch (displayevent)
      {
      case DispState::NO_EVENT:
        break;
      case DispState::UPDATE:
        DisplayState::update_display = true;
        break;
      default:
        DisplayState::lastDispEvent = displayevent;
        DisplayState::dispFsm.trigger(displayevent);
        break;
      }

      ButtonClickType buttonEvent = buttonQueue->read<ButtonClickType>();
      switch (buttonEvent)
      {
      case NO_CLICK:
        break;
      case SINGLE:
        DisplayState::lastDispEvent = DispState::PRIMARY_SINGLE_CLICK;
        DisplayState::dispFsm.trigger(DispState::PRIMARY_SINGLE_CLICK);
        break;
      case DOUBLE:
        DisplayState::lastDispEvent = DispState::PRIMARY_DOUBLE_CLICK;
        DisplayState::dispFsm.trigger(DispState::PRIMARY_DOUBLE_CLICK);
        break;
      case TRIPLE:
        DisplayState::lastDispEvent = DispState::PRIMARY_TRIPLE_CLICK;
        DisplayState::dispFsm.trigger(DispState::PRIMARY_TRIPLE_CLICK);
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