
bool display_task_initialised = false;

#ifndef TFT_H
#include <tft.h>
#endif

void display_task_0(void *pvParameters)
{
  elapsedMillis since_updated_display;

  setupLCD();

  add_disp_state_transitions();

  Serial.printf("display_task_0 running on core %d\n", xPortGetCoreID());

  display_state.run_machine();

  display_task_initialised = true;

  elapsedMillis since_read_disp_event_queue;

  while (true)
  {

    if (since_read_disp_event_queue > 100)
    {
      since_read_disp_event_queue = 0;
      display_state.run_machine();

      DispStateEvent ev = read_from_display_event_queue();
      if (ev == DISP_EV_REFRESH)
      {
        if (since_updated_display > 500)
        {
          since_updated_display = 0;
          display_state_event(DISP_EV_REFRESH);
        }
        else
        {
          DEBUG("too soon!");
        }
      }
      else if (ev != DISP_EV_NO_EVENT)
      {
        DEBUGVAL("ev", ev);
        display_state_event(ev);
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
