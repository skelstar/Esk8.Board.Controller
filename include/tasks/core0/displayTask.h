
#ifndef TFT_H
#include <tft.h>
#endif

#include <statsClass.h>

namespace Display
{
  void printState(uint16_t id);
  void printTrigger(uint16_t ev);

  StatsClass *_stats;

  bool taskReady = false;

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Display", xPortGetCoreID());

    setupLCD();

    dispFsm.begin(&fsm);
    dispFsm.setPrintStateCallback(printState);
    dispFsm.setPrintTriggerCallback(printTrigger);

    addTransitions();

    fsm.run_machine();

    taskReady = true;

#define READ_DISP_EVENT_QUEUE_PERIOD 100

    elapsedMillis sinceReadDispEventQueue, since_checked_queues;

    while (true)
    {
      if (since_checked_queue > 500)
      {
        since_checked_queue = 0;

        _stats = statsQueue->peek<StatsClass>();
      }

      if (sinceReadDispEventQueue > READ_DISP_EVENT_QUEUE_PERIOD)
      {
        sinceReadDispEventQueue = 0;
        fsm.run_machine();

        DispState::Trigger displayevent = displayQueue->read<DispState::Trigger>();
        switch (displayevent)
        {
        case DispState::NO_EVENT:
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
        case LONG_PRESS:
          lastDispEvent = DispState::PRIMARY_LONG_PRESS;
          dispFsm.trigger(DispState::PRIMARY_LONG_PRESS);
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

  void printState(uint16_t id)
  {
    if (PRINT_DISP_STATE)
      Serial.printf(PRINT_STATE_FORMAT, "DISP", Display::stateID(id));
  }

  void printTrigger(uint16_t ev)
  {
    if (PRINT_DISP_STATE_EVENT &&
        !(fsm.revisit() && ev == DispState::STOPPED) &&
        !(fsm.revisit() && ev == DispState::MOVING))
      Serial.printf(PRINT_sFSM_sTRIGGER_FORMAT, "DISP", DispState::getTrigger(ev));
  }

} // namespace Display