
#ifndef TFT_H
#include <tft.h>
#endif

namespace Display
{
  void task(void *pvParameters)
  {
    setupLCD();

    dispFsm.begin(&fsm, DISP_STATE_FORMAT, DISP_STATE_STRING_FORMAT);
    dispFsm.setGetStateNameCallback([](uint16_t id) {
      return getStateName(id);
    });
    // dispFsm.setGetEventNameCallback([](uint8_t ev) {
    //   return DispState::getEvent(ev);)
    // });
    addTransitions();

    Serial.printf("displayTask running on core %d\n", xPortGetCoreID());

    fsm.run_machine();

    display_task_initialised = true;

#define READ_DISP_EVENT_QUEUE_PERIOD 100

    elapsedMillis sinceReadDispEventQueue;

    while (true)
    {
      if (sinceReadDispEventQueue > READ_DISP_EVENT_QUEUE_PERIOD)
      {
        sinceReadDispEventQueue = 0;
        fsm.run_machine();

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
          dispFsm.trigger(displayevent);
          break;
        }

        ButtonClickType buttonEvent = buttonQueue->read<ButtonClickType>();
        switch (buttonEvent)
        {
        case NO_CLICK:
          break;
        case SINGLE:
          lastDispEvent = DispState::PRIMARY_SINGLE_CLICK;
          dispFsm.trigger(DispState::PRIMARY_SINGLE_CLICK);
          break;
        case DOUBLE:
          lastDispEvent = DispState::PRIMARY_DOUBLE_CLICK;
          dispFsm.trigger(DispState::PRIMARY_DOUBLE_CLICK);
          break;
        case TRIPLE:
          lastDispEvent = DispState::PRIMARY_TRIPLE_CLICK;
          dispFsm.trigger(DispState::PRIMARY_TRIPLE_CLICK);
          break;
        }
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //-----------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "displayTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }
}