
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

      DispStateEvent ev = read_from_display_event_queue();
      switch (ev)
      {
      case DISP_EV_NO_EVENT:
        break;
      case DISP_EV_UPDATE:
        update_display = true;
        break;
      default:
        lastDispEvent = ev;
        display_state->trigger(ev);
        break;
      }

      uint8_t buttonEvent = buttonQueueManager->read();
      switch (buttonEvent)
      {
      case TRIPLE:
        Serial.printf("Triple click event received\n");
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
