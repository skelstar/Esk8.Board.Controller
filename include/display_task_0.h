
void display_task_0(void *pvParameters)
{
  elapsedMillis since_updated_display;

  setupLCD();

  add_disp_state_transitions();

  Serial.printf("display_task_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    display_state.run_machine();

    DispStateEvent ev = read_from_display_event_queue();
    if (ev == DISP_EV_REFRESH)
    {
      since_updated_display = 0;
      display_state_event(DISP_EV_REFRESH);
    }
    else if (ev != DISP_EV_NO_EVENT)
    {
      display_state_event(ev);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
