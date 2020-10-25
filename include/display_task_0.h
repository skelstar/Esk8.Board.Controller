
#ifndef TFT_H
#include <tft.h>
#endif

void display_task_0(void *pvParameters)
{
  setupLCD();

  display_state = new Fsm(&disp_state_searching);

  add_disp_state_transitions();

  Serial.printf("display_task_0 running on core %d\n", xPortGetCoreID());

  display_state->run_machine();

  display_task_initialised = true;

#define READ_DISP_EVENT_QUEUE_PERIOD 100

  elapsedMillis since_read_disp_event_queue;

  while (true)
  {
    if (since_read_disp_event_queue > READ_DISP_EVENT_QUEUE_PERIOD)
    {
      since_read_disp_event_queue = 0;
      display_state->run_machine();

      DispStateEvent displayevent = (DispStateEvent)displayChangeQueueManager->read(); // read_from_display_event_queue();
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
        Serial.printf("Read, now triggering: %d\n", eventToString(displayevent));
        display_state->trigger(displayevent);
        break;
      }

      uint8_t buttonEvent = buttonQueueManager->read();
      switch (buttonEvent)
      {
      case SINGLE:
        lastDispEvent = DISP_EV_PRIMARY_SINGLE_CLICK;
        display_state->trigger(DISP_EV_PRIMARY_SINGLE_CLICK);
        break;
      case DOUBLE:
        lastDispEvent = DISP_EV_PRIMARY_DOUBLE_CLICK;
        display_state->trigger(DISP_EV_PRIMARY_DOUBLE_CLICK);
        break;
      case TRIPLE:
        lastDispEvent = DISP_EV_PRIMARY_TRIPLE_CLICK;
        display_state->trigger(DISP_EV_PRIMARY_TRIPLE_CLICK);
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
