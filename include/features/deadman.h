#ifndef Button2
#include <Button2.h>
#endif

// prototypes
void send_to_deadman_event_queue(DeadmanEvent e);

//----------------------------------------------------------

void deadmanTask_0(void *pvParameters)
{
  Button2 deadman(DEADMAN_PIN);

  Serial.printf("deadmanTask_0 running on core %d\n", xPortGetCoreID());

  deadman.setPressedHandler([](Button2 &btn) {
    send_to_deadman_event_queue(EV_DEADMAN_PRESSED);
  });
  deadman.setReleasedHandler([](Button2 &btn) {
    send_to_deadman_event_queue(EV_DEADMAN_RELEASED);
  });

  while (true)
  {
    deadman.loop();
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//---------------------------------------------------------

DeadmanEvent read_from_deadman_event_queue()
{
  DeadmanEvent e;
  if (xDeadmanQueueEvent != NULL && xQueueReceive(xDeadmanQueueEvent, &e, (TickType_t)5) == pdPASS)
  {
    if (e == EV_DEADMAN_NO_EVENT)
    {
      // error
      DEBUG("ERROR: EV_DEADMAN_NO_EVENT received!");
    }
    return e;
  }
  return EV_DEADMAN_NO_EVENT;
}
//---------------------------------------------------------

void send_to_deadman_event_queue(DeadmanEvent ev)
{
  xQueueSendToFront(xDeadmanQueueEvent, &ev, pdMS_TO_TICKS(10));
}
//---------------------------------------------------------

