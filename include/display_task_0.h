
#ifndef TFT_H
#include <tft.h>
#endif

void display_task_0(void *pvParameters)
{
  RTOSUtils::printTaskDetails();

  setupLCD();

  dispFsm.begin(&Disp::fsm, ">State: %s | %s\n");
  dispFsm.setGetStateNameCallback([](uint8_t id) {
    return Disp::stateIDsMgr.getName(id);
  });
  dispFsm.setGetEventNameCallback([](uint8_t ev) {
    return Disp::eventsMgr.getName(ev);
  });

  Disp::addTransitions();

  Disp::fsm.run_machine();

  display_task_initialised = true;

#define READ_DISP_EVENT_QUEUE_PERIOD 100

  elapsedMillis sinceReadDispEventQueue;

  while (true)
  {
    if (sinceReadDispEventQueue > READ_DISP_EVENT_QUEUE_PERIOD)
    {
      sinceReadDispEventQueue = 0;
      Disp::fsm.run_machine();

      uint8_t ev = displayChangeQueueManager->read();
      if (ev >= Disp::EventLength && ev != NO_QUEUE_EVENT)
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
        dispFsm.trigger(ev);
        break;
      }

      uint8_t buttonEvent = buttonQueueManager->read();
      switch (buttonEvent)
      {
      case SINGLE:
        dispFsm.trigger(Disp::PRIMARY_SINGLE_CLICK);
        break;
      case DOUBLE:
        dispFsm.trigger(Disp::PRIMARY_DOUBLE_CLICK);
        break;
      case TRIPLE:
        dispFsm.trigger(Disp::PRIMARY_TRIPLE_CLICK);
        break;
      case NO_EVENT:
        break;
      }
    }

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
