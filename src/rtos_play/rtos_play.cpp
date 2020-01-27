#include <Arduino.h>

#include <elapsedMillis.h>

xQueueHandle xQueue1;
xQueueHandle xQueue2;

SemaphoreHandle_t x_Semaphore1;
// SemaphoreHandle_t xSemaphore2;


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
  elapsedMillis since_task_1 = 5000;

  while (true)
  {
    if (since_task_1 > 10000)
    {
      Serial.printf("task_1 at %lu\n", since_task_1);
      since_task_1 = 0;

      Serial.printf("starting take (task_1)\n");
      if (x_Semaphore1 != NULL && xSemaphoreTake(x_Semaphore1, (TickType_t) 10) == pdTRUE)
      {
        vTaskDelay(4000);
        xSemaphoreGive(x_Semaphore1);
      }
      // read_from_queue1(2000);
      Serial.printf("finished take (task_1)\n");
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
      Serial.printf("task_2 at %lu\n", since_task_2);
      since_task_2 = 0;

      if (x_Semaphore1 != NULL && xSemaphoreTake(x_Semaphore1, (TickType_t) 10) == pdTRUE)
      {
        Serial.printf("Got semaphore task_2\n");
        vTaskDelay(2000);
        xSemaphoreGive(x_Semaphore1);
        Serial.printf("Finished with semaphore task_2\n");
      }
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

  x_Semaphore1 = xSemaphoreCreateMutex();
  // xSemaphore2 = xSemaphoreCreateMutex();
}

void loop()
{
  vTaskDelay(10);
}
//---------------------------------------------------------------------
