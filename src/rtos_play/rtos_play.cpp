#include <Arduino.h>

#include <elapsedMillis.h>
//---------------------------------------------------------------------
class TaskConfig
{
  public:

    TaskConfig(unsigned long f, unsigned long b, unsigned long w)
    {
      freq = f;
      blocking_period = b;
      wait_ticks = w;
    }

    unsigned long freq;
    unsigned long blocking_period;
    unsigned long wait_ticks;
};
//---------------------------------------------------------------------

xQueueHandle xQueue1;
xQueueHandle xQueue2;

SemaphoreHandle_t xTheSemaphore;
// SemaphoreHandle_t xSemaphore2;

TaskConfig task1_config = TaskConfig(1000, 100, 100);
TaskConfig task2_config = TaskConfig(3000, 100, 100);

xTimerHandle xTimer1;

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
bool waiting_for_semaphore(SemaphoreHandle_t sempahore, TickType_t ticks)
{
  /* 
  tried to take the semaphore for x ticks then returns the result. If it can't
  take the semaphore then it returns false, true otherwise
  */
  return sempahore == NULL || xSemaphoreTake(sempahore, ticks) != pdTRUE;
}
//---------------------------------------------------------------------
void task_1(void *pvParameters)
{
  elapsedMillis since_took_semaphore_task_1;
  Serial.printf("task_1 running on core %d\n", xPortGetCoreID());
  TaskConfig config = *((TaskConfig*) pvParameters);

  while (true)
  {
    if (since_took_semaphore_task_1 > config.freq)
    {
      since_took_semaphore_task_1 = 0;

      while (waiting_for_semaphore(xTheSemaphore, config.wait_ticks))
      {
        Serial.printf(" XXX --> task_1 blocked!\n");
      }

      Serial.printf("--> task_1 took semaphore\n");
      elapsedMillis since_started_task_0_loop = 0;
      while (since_started_task_0_loop < config.blocking_period)
      {
        /*
        if you remove the taskDelay, then other threads will not be switched to
        by the task scheduler until the vTaskDelay(10) below
        */
        vTaskDelay(1);
      }
      xSemaphoreGive(xTheSemaphore);
      Serial.printf("<-- task_1 giving semaphore after %lums\n", (unsigned long)since_started_task_0_loop);
    }
    vTaskDelay(10);  // <--- task exits here (I think)
  }
  vTaskDelete(NULL);
}
//---------------------------------------------------------------------
void task_2(void *pvParameters)
{
  Serial.printf("task_2 running on core %d\n", xPortGetCoreID());

  elapsedMillis task_2_try_take_semaphore = 0;
  TaskConfig config = *((TaskConfig*) pvParameters);

  while (true)
  {
    if (task_2_try_take_semaphore > config.freq)
    {
      task_2_try_take_semaphore = 0;

      while (waiting_for_semaphore(xTheSemaphore, /*ticks*/config.wait_ticks))
      {
        Serial.printf(" XXX --> task_2 blocked!\n");
      }

      Serial.printf("--> task_2 took semaphore\n");
      elapsedMillis since_started_task_2_loop = 0;
      while (since_started_task_2_loop < config.blocking_period)
      {
        /*
        if you remove the taskDelay, then other threads will not be switched to
        by the task scheduler until the vTaskDelay(10) below
        */
        vTaskDelay(1);
      }
      Serial.printf("<== task_2 giving semaphore after %lums\n", (unsigned long)since_started_task_2_loop);
      xSemaphoreGive(xTheSemaphore);
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
      // Serial.printf("--------------------------\nSeeding queue: %d\n", seed_val);
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
      if (xQueuePeek(xQueue1, &peeked_val, (TickType_t)10))
      {
        // Serial.printf("Peeking queue: %d\n", peeked_val);
      }
      else
      {
        // Serial.printf("FAILED to peek queue\n");
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
      // Serial.printf("-------> read val: %d\n", read_val);
    }
    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//---------------------------------------------------------------------

elapsedMillis since_last_timer1 = 0;

void vTimer1_cb(TimerHandle_t xTimer)
{
  if (since_last_timer1 > xTimerGetPeriod(xTimer) + 10)
  {
    Serial.printf("Timer1! %lu\n", (unsigned long)since_last_timer1);
  }
  since_last_timer1 = 0;
}
//---------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  xTaskCreatePinnedToCore(task_1, "task_1", 4092, (void *) &task1_config, /*priority*/ 4, NULL, 1);
  xTaskCreatePinnedToCore(task_2, "task_2", 4092, (void *) &task2_config, /*priority*/ 3, NULL, 1);
  xTaskCreatePinnedToCore(task_queue_seeder, "task_queue_seeder", 4092, NULL, /*priority*/ 2, NULL, 1);
  xTaskCreatePinnedToCore(task_queue_peeker, "task_queue_peeker", 4092, NULL, /*priority*/ 2, NULL, 1);
  xTaskCreatePinnedToCore(task_queue_reader, "task_queue_reader", 4092, NULL, /*priority*/ 2, NULL, 1);

  xTimer1 = xTimerCreate(
      "xTimer1",
      /*ticks*/ 500,
      pdTRUE,
      (void *)0,
      vTimer1_cb);

  if (xTimer1 != NULL)
  {
    if (xTimerStart(xTimer1, 0) != pdPASS)
    {
      Serial.printf("xTimer1 could not be started\n");
    }
  }

  xQueue1 = xQueueCreate(1, sizeof(uint8_t));
  xQueue2 = xQueueCreate(1, sizeof(uint8_t));

  xTheSemaphore = xSemaphoreCreateMutex();
}

void loop()
{
  vTaskDelay(10);
}
//---------------------------------------------------------------------
