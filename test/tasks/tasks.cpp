#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_STATS_MUTEX_TAKE_STATE 1

#include <rtosManager.h>
#include <QueueManager.h>
#include <SparkFun_Qwiic_Button.h>
#include <elapsedMillis.h>

MyMutex mutex_I2C;

xQueueHandle queueHandle;
Queue::Manager *queue;

#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"

#define TICKS_5 5
#define TICKS_10 10
#define TICKS_50 50

#include <tasks/core0/qwiicButtonTask.h>
#include <tasks/core0/NintendoClassicTask.h>

#define CORE_0 0
#define PRIORITY_1 1

elapsedMillis since_checked_queue;

// runs every test
void setUp()
{
}

// runs every test
void tearDown()
{
}

void test_qwiic_button_pressed_then_released_via_queue()
{
  Wire.begin(); //Join I2C bus

  QwiickButtonState *actual;

  mutex_I2C.create("i2c", /*default*/ TICKS_5);
  mutex_I2C.enabled = true;

  QwiicButtonTask::createTask(CORE_0, PRIORITY_1);

  while (!QwiicButtonTask::taskReady)
  {
    vTaskDelay(5);
  }

  Serial.printf("Press then release the Qwiic button to satify test\n");

  bool was_pressed = false, was_released = false;
  while (!(was_pressed && was_released))
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;
      actual = QwiicButtonTask::queue->peek<QwiickButtonState>(__func__);
      if (!was_pressed && actual != nullptr && actual->pressed)
      {
        Serial.printf("Qwiic button pressed\n");
        was_pressed = true;
      }
      else if (was_pressed && actual != nullptr && !actual->pressed)
      {
        Serial.printf("Qwiic button released\n");
        was_released = true;
      }
    }
    vTaskDelay(5);
  }
  TEST_ASSERT_TRUE(was_pressed && was_released);
}

void test_nintendo_button_task()
{
  Wire.begin(); //Join I2C bus

  NintendoButtonEvent *actual;

  mutex_I2C.create("i2c", /*default*/ TICKS_5);
  mutex_I2C.enabled = true;

  NintendoClassicTask::createTask(CORE_0, PRIORITY_1);

  while (!NintendoClassicTask::taskReady)
  {
    vTaskDelay(5);
  }

  Serial.printf("Press a button to satify test\n");

  bool was_pressed = false, was_released = false;

  while (!(was_pressed && was_released))
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;
      actual = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);
      if (!was_pressed && actual != nullptr && actual->state == 1)
      {
        Serial.printf("Nintendo button pressed %d %d\n");
        was_pressed = true;
      }
      else if (was_pressed && actual != nullptr && actual->state == 0)
      {
        Serial.printf("Nintendo button released\n");
        was_released = true;
      }
    }
    vTaskDelay(5);
  }
  TEST_ASSERT_TRUE(was_pressed && was_released);
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  RUN_TEST(test_qwiic_button_pressed_then_released_via_queue);
  RUN_TEST(test_nintendo_button_task);

  UNITY_END();
}

void loop()
{
}
