
#ifndef TFT_H
#include <tft.h>
#endif

void display_task_0(void *pvParameters)
{
  setupLCD();

  add_disp_state_transitions();

  Serial.printf("display_task_0 running on core %d\n", xPortGetCoreID());

  display_state.run_machine();

  display_task_initialised = true;

#define READ_DISP_EVENT_QUEUE_PERIOD 100

  elapsedMillis since_read_disp_event_queue;

  while (true)
  {

    if (since_read_disp_event_queue > READ_DISP_EVENT_QUEUE_PERIOD)
    {
      since_read_disp_event_queue = 0;
      display_state.run_machine();

      DispStateEvent ev = read_from_display_event_queue();
      if (ev != DISP_EV_NO_EVENT)
      {
        lastDispEvent = ev;
        display_state.trigger(ev);
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
