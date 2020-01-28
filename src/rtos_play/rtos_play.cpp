#include <Arduino.h>

#include <elapsedMillis.h>

xQueueHandle xQueue1;
xQueueHandle xQueue2;

SemaphoreHandle_t xTheSemaphore;
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
  unsigned long task_1_now = millis();

  while (true)
  {
    if (millis() - task_1_now > 10000)
    {
      task_1_now = millis();

      if (xTheSemaphore != NULL && xSemaphoreTake(xTheSemaphore, (TickType_t) 10) == pdTRUE)
      {
        Serial.printf("--> task_1 took semaphore\n");
        vTaskDelay(4000);
        Serial.printf("<-- task_1 giving semaphore after %lums\n", millis() - task_1_now);
        xSemaphoreGive(xTheSemaphore);
      }
      else {
        Serial.printf(" XXX --> task_1 blocked!\n");
      }
      // read_from_queue1(2000);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//---------------------------------------------------------------------
void task_2(void *pvParameters)
{
  Serial.printf("task_2 running on core %d\n", xPortGetCoreID());

  unsigned long task_2_now = 0;

  while (true)
  {
    if (millis() - task_2_now > 3000)
    {
      task_2_now = millis();

      if (xTheSemaphore != NULL && xSemaphoreTake(xTheSemaphore, (TickType_t) 10) == pdTRUE)
      {
        Serial.printf("==> task_2 took semaphore\n");
        vTaskDelay(2000);
        Serial.printf("<== task_2 giving semaphore after %lums\n", millis() - task_2_now);
        xSemaphoreGive(xTheSemaphore);
      }
      else {
        Serial.printf(" XXX --> task_2 blocked!\n");
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

//---------------------------------------------------------------------
void task_queue_seeder(void *pvParameters)
{
  Serial.printf("task_queue_seeder running on core %d\n", xPortGetCoreID());

  elapsedMillis since_seeded_queue;
  uint8_t seed_val = 0;

  while (true)
  {
    if (since_seeded_queue > 5000)
    {
      since_seeded_queue = 0;
      Serial.printf("--------------------------\nSeeding queue: %d\n", seed_val);
      send_to_queue1(seed_val);
      seed_val++;
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//---------------------------------------------------------------------
void task_queue_peeker(void *pvParameters)
{
  Serial.printf("task_queue_peeker running on core %d\n", xPortGetCoreID());

  elapsedMillis since_peeked_queue;
  uint8_t peeked_val = 0;

  while (true)
  {
    if (since_peeked_queue > 1000)
    {
      since_peeked_queue = 0;
      if (xQueuePeek(xQueue1, &peeked_val, (TickType_t) 10))
      {
        Serial.printf("Peeking queue: %d\n", peeked_val);
      }
      else 
      {
        Serial.printf("FAILED to peek queue\n");
      }
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//---------------------------------------------------------------------
void task_queue_reader(void *pvParameters)
{
  Serial.printf("task_queue_reader running on core %d\n", xPortGetCoreID());

  elapsedMillis since_read_queue;
  uint8_t read_val = 0;

  while (true)
  {
    if (since_read_queue > 4000)
    {
      since_read_queue = 0;
      read_val = read_from_queue1(10);
      Serial.printf("-------> read val: %d\n", read_val);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

//---------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  // xTaskCreatePinnedToCore(task_1, "task_1", 4092, NULL, /*priority*/ 4, NULL, 1);
  // xTaskCreatePinnedToCore(task_2, "task_2", 4092, NULL, /*priority*/ 3, NULL, 1);
  xTaskCreatePinnedToCore(task_queue_seeder, "task_queue_seeder", 4092, NULL, /*priority*/ 2, NULL, 1);
  xTaskCreatePinnedToCore(task_queue_peeker, "task_queue_peeker", 4092, NULL, /*priority*/ 2, NULL, 1);
  xTaskCreatePinnedToCore(task_queue_reader, "task_queue_reader", 4092, NULL, /*priority*/ 2, NULL, 1);

  xQueue1 = xQueueCreate(1, sizeof(uint8_t));
  xQueue2 = xQueueCreate(1, sizeof(uint8_t));

  xTheSemaphore = xSemaphoreCreateMutex();
  // xSemaphore2 = xSemaphoreCreateMutex();
}

void loop()
{
  vTaskDelay(10);
}
//---------------------------------------------------------------------
