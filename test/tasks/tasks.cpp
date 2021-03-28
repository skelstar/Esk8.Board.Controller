#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_STATS_MUTEX_TAKE_STATE 1
#define PRINT_MUTEX_TAKE_FAIL 1

#include <rtosManager.h>
#include <QueueManager.h>
#include <SparkFun_Qwiic_Button.h>
#include <elapsedMillis.h>

MyMutex mutex_I2C;

xQueueHandle queueHandle;
Queue::Manager *queue;

#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define PRINT_THROTTLE 0

#define TICKS_5ms 5 / portTICK_PERIOD_MS
#define TICKS_10ms 10 / portTICK_PERIOD_MS
#define TICKS_50ms 50 / portTICK_PERIOD_MS
#define TICKS_100ms 100 / portTICK_PERIOD_MS

#include <tasks/core0/qwiicButtonTask.h>
#include <tasks/core0/NintendoClassicTask.h>
#include <tasks/core0/ThrottleTask.h>
#include <tasks/core0/debugTask.h>

#define CORE_0 0
#define PRIORITY_0 0
#define PRIORITY_1 1
#define PRIORITY_2 2
#define PRIORITY_3 3
#define PRIORITY_4 4

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

  QwiicButtonState *actual;

  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
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
      actual = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);
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

void test_nintendo_button_is_pressed_then_released_task()
{
  Wire.begin(); //Join I2C bus
  Wire.setClock(200000);

  NintendoButtonEvent *actual;

  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
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

void test_magnetic_throttle_is_moved_greater_than_220()
{
  Wire.begin(); //Join I2C bus

  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  QwiicButtonTask::createTask(CORE_0, PRIORITY_1);
  ThrottleTask::createTask(CORE_0, PRIORITY_1);

  while (!ThrottleTask::taskReady)
  {
    vTaskDelay(5);
  }

  Serial.printf("Move throttle to >220 degrees to satify test\n");

  uint8_t throttle = 127;
  unsigned long last_id = 0;

  while (throttle < 220)
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;

      ThrottleState *actual = ThrottleTask::queue->peek<ThrottleState>(__func__);

      if (actual != nullptr && !actual->been_peeked(last_id) && throttle != actual->val)
      {
        Serial.printf("Throttle changed %d\n", actual->val);
        throttle = actual->val;
        last_id = actual->id;
      }
    }

    vTaskDelay(5);
  }

  TEST_ASSERT_TRUE(throttle > 220);
}

void test_run_debug_task()
{
  Wire.begin(); //Join I2C bus

  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  QwiicButtonTask::createTask(CORE_0, PRIORITY_3);
  ThrottleTask::createTask(CORE_0, PRIORITY_2);
  NintendoClassicTask::createTask(CORE_0, PRIORITY_2);
  Debug::createTask(CORE_0, PRIORITY_0);

  while (!ThrottleTask::taskReady || !Debug::taskReady)
  {
    vTaskDelay(5);
  }

  Serial.printf("Watch then click the button to end the test\n");

  while (1)
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;

      QwiicButtonState *button = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);

      if (button != nullptr && button->pressed)
      {
        TEST_ASSERT_TRUE(button->pressed);
      }
    }

    vTaskDelay(5);
  }
}

void test_run_all_tasks()
{
  Wire.begin(); //Join I2C bus
  // Wire.begin(200000); //Join I2C bus

  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  QwiicButtonTask::createTask(CORE_0, PRIORITY_3);
  ThrottleTask::createTask(CORE_0, PRIORITY_2);
  NintendoClassicTask::createTask(CORE_0, PRIORITY_1);
  Debug::createTask(CORE_0, PRIORITY_0);

  while (!ThrottleTask::taskReady || !Debug::taskReady)
  {
    vTaskDelay(5);
  }

  Serial.printf("Watch then click the button to end the test\n");

  while (1)
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;

      QwiicButtonState *button = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);

      if (button != nullptr && button->pressed)
      {
        TEST_ASSERT_TRUE(button->pressed);
      }
    }

    vTaskDelay(5);
  }
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  // RUN_TEST(test_qwiic_button_pressed_then_released_via_queue);
  RUN_TEST(test_nintendo_button_is_pressed_then_released_task);
  // RUN_TEST(test_magnetic_throttle_is_moved_greater_than_220);
  // RUN_TEST(test_run_debug_task);
  RUN_TEST(test_run_all_tasks);

  UNITY_END();
}

void loop()
{
}
