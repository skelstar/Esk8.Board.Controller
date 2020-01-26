#include <Arduino.h>

#include <elapsedMillis.h>

xQueueHandle xQueue1;
xQueueHandle xQueue2;

//---------------------------------------------------------------------
void send_to_queue1(uint8_t ev)
{
  xQueueSendToFront(xQueue1, &ev, pdMS_TO_TICKS(10));
}

void send_to_queue2(uint8_t ev)
{
  xQueueSendToFront(xQueue2, &ev, pdMS_TO_TICKS(10));
}
//---------------------------------------------------------------------
uint8_t read_from_queue1(TickType_t ticks)
{
  uint8_t e;
  if (xQueue1 != NULL && xQueueReceive(xQueue1, &e, ticks) == pdPASS)
  {
    return e;
  }
  return 0;
}

uint8_t read_from_queue2(TickType_t ticks)
{
  uint8_t e;
  if (xQueue2 != NULL && xQueueReceive(xQueue2, &e, ticks) == pdPASS)
  {
    return e;
  }
  return 0;
}
//---------------------------------------------------------------------
void task_1(void *pvParameters)
{
  Serial.printf("task_1 running on core %d\n", xPortGetCoreID());
  elapsedMillis since_task_1 = 0;

  while (true)
  {
    if (since_task_1 > 2300)
    {
      since_task_1 = 0;
      Serial.printf("task_1\n");

      Serial.printf("starting read (task_1)\n");
      read_from_queue1(2000);
      Serial.printf("finished read (task_1)\n");

      // Serial.printf("starting 4s delay (task_1)\n");
      // delay(4000);
      // Serial.printf("finished 4s delay (task_1)\n");
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//---------------------------------------------------------------------
void task_2(void *pvParameters)
{
  elapsedMillis since_task_2 = 0;
  Serial.printf("task_2 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    if (since_task_2 > 1000)
    {
      since_task_2 = 0;
      Serial.printf("task_2\n");
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

//---------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  xTaskCreatePinnedToCore(task_1, "task_1", 4092, NULL, /*priority*/ 4, NULL, 1);
  xTaskCreatePinnedToCore(task_2, "task_2", 4092, NULL, /*priority*/ 3, NULL, 1);

  xQueue1 = xQueueCreate(1, sizeof(uint8_t));
  xQueue2 = xQueueCreate(1, sizeof(uint8_t));
}

void loop()
{
  vTaskDelay(10);
}
//---------------------------------------------------------------------
